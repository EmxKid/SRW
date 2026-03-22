#include "Simulator.h"
#include "RandomGenerator.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <iostream>

Simulator::Simulator(
    int maxUsers,
    double baseServiceRate,
    std::unique_ptr<Distribution> workloadDist,
    std::unique_ptr<Distribution> passiveTimeDist,
    std::unique_ptr<Distribution> serviceTimeDist,
    std::function<double(double)> degradationFn
) : m_users(maxUsers),
    m_currentTime(0.0),
    m_statUpdateTime(0.0),
    m_baseServiceRate(baseServiceRate),
    m_currentEffectiveRate(baseServiceRate),
    m_workloadDist(std::move(workloadDist)),
    m_serviceTime(std::move(serviceTimeDist)),
    m_passiveTimeDist(std::move(passiveTimeDist)),
    m_degradationFn(degradationFn),
    m_userStates(maxUsers, false),
    m_lastEventTime(maxUsers, 0.0),
    m_Workload(maxUsers, 0.0),
    m_remainingTime(maxUsers, 0.0),
    m_eventVersion(maxUsers, 0),
    m_stats(maxUsers),
    m_eventQueue()
{
    if (!m_workloadDist || !m_passiveTimeDist)
        throw std::invalid_argument("Distributions cannot be null");
    if (m_baseServiceRate <= 0.0)
        throw std::invalid_argument("Base service rate must be positive");
}

void Simulator::initialize() {
    for (int userId = 0; userId < m_users; ++userId) {
        double nextActivation = m_currentTime + m_passiveTimeDist->sample();
        m_eventVersion[userId]++;
        m_eventQueue.push(
            nextActivation,
            EventType::ACTIVATION,
            userId,
            m_eventVersion[userId],
            [this, userId]() { handleActivation(userId); }
        );
        m_lastEventTime[userId] = m_currentTime;
    }
    scheduleMonitoring(m_currentTime + 1.0);
}

void Simulator::scheduleMonitoring(double nextTime) {
    if (nextTime <= m_currentTime) return;
    m_eventQueue.push(
        nextTime,
        EventType::MONITORING,
        -1,
        0,
        [this, nextTime]() {
            handleMonitoring();
            scheduleMonitoring(nextTime + 1.0);
        }
    );
}

void Simulator::handleActivation(int userId) {
    if (userId < 0 || userId >= m_users) {
        std::cerr << "[ERROR] Invalid userId=" << userId << " in handleActivation\n";
        return;
    }
    
    updateGlobalStatistics(m_currentTime);
    updateStatistics(userId, m_currentTime);
    
    double oldTotalWorkload = getTotalWorkload();
    double oldRate = computeEffectiveRate(oldTotalWorkload);
    
    double workload = m_workloadDist->sample();
    m_Workload[userId] = workload;
    
    double newTotalWorkload = oldTotalWorkload + workload;
    double newRate = computeEffectiveRate(newTotalWorkload);
    
    rescaleRemainingTimes(oldRate, newRate, -1);
    
    m_userStates[userId] = true;
    
    double initialTime = m_serviceTime->sample(newRate);
    
    m_remainingTime[userId] = initialTime;
    
    m_eventVersion[userId]++;
    m_eventQueue.push(
        m_currentTime + initialTime,
        EventType::DEACTIVATION,
        userId,
        m_eventVersion[userId],
        [this, userId]() { handleDeactivation(userId); }
    );
    
    int activeCount = std::count(m_userStates.begin(), m_userStates.end(), true);
    m_stats.maxConcurrentUsers = std::max(m_stats.maxConcurrentUsers, activeCount);
    
    m_currentEffectiveRate = newRate;
    m_stats.recordDegradation(newRate / m_baseServiceRate);
}

void Simulator::handleDeactivation(int userId) {
    if (userId < 0 || userId >= m_users) {
        std::cerr << "[ERROR] Invalid userId=" << userId << " in handleDeactivation\n";
        return;
    }
    
    updateGlobalStatistics(m_currentTime);
    updateStatistics(userId, m_currentTime);
    
    double oldTotalWorkload = getTotalWorkload();
    double oldRate = computeEffectiveRate(oldTotalWorkload);
    
    double freedWorkload = m_Workload[userId];
    m_Workload[userId] = 0.0;
    m_remainingTime[userId] = 0.0;
    
    double newTotalWorkload = oldTotalWorkload - freedWorkload;
    double newRate = computeEffectiveRate(newTotalWorkload);
    
    rescaleRemainingTimes(oldRate, newRate, userId);
    
    m_userStates[userId] = false;
    
    m_stats.taskCount[userId]++;
    m_stats.totalWorkCompleted[userId] += freedWorkload;
    m_stats.totalWorkProcessed += freedWorkload;
    
    double completionTime = freedWorkload / oldRate;
    int bucket = static_cast<int>(completionTime * 10);
    m_stats.completionTimeHistogram[bucket] += 1.0;
    
    double nextPassive = m_passiveTimeDist->sample();
    m_eventVersion[userId]++;
    m_eventQueue.push(
        m_currentTime + nextPassive,
        EventType::ACTIVATION,
        userId,
        m_eventVersion[userId],
        [this, userId]() { handleActivation(userId); }
    );
    
    m_currentEffectiveRate = newRate;
}

void Simulator::runUntil(double endTime) {
    if (endTime <= 0.0)
        throw std::invalid_argument("Simulation time must be > 0");
    
    m_statUpdateTime = 0.0;
    m_currentTime = 0.0;
    initialize();
    
    while (!m_eventQueue.empty() && m_currentTime < endTime) {
        Event event = m_eventQueue.pop();
        
        if (event.userId >= 0 && event.userId < m_users) {
            if (event.eventVersion < m_eventVersion[event.userId]) {
                continue;
            }
        }
        
        m_currentTime = event.time;
        event.handler();
        m_stats.totalEventsProcessed++;
    }
    
    for (int userId = 0; userId < m_users; ++userId) {
        updateStatistics(userId, endTime);
    }
    updateGlobalStatistics(endTime);
    m_stats.totalSimulationTime = endTime;
}

void Simulator::attachListener(ISimulationListener* listener) {
    if (listener) {
        m_listeners.push_back(listener);
    }
}

void Simulator::notifyListeners() {
    if (m_listeners.empty()) return;

    // Собираем данные в структуру (Simulator знает свои данные, но не знает, куда они пойдут)
    SimulationSnapshot snapshot;
    snapshot.time = m_currentTime;
    snapshot.activeUsers = std::count(m_userStates.begin(), m_userStates.end(), true);
    snapshot.totalWorkload = getTotalWorkload();
    snapshot.effectiveRate = m_currentEffectiveRate; // или computeEffectiveRate(...)
    snapshot.degradationFactor = snapshot.effectiveRate / m_baseServiceRate;

    // Рассылаем всем подписчикам
    for (auto* listener : m_listeners) {
        listener->onSnapshot(snapshot);
    }
}

void Simulator::handleMonitoring() {
    // Вызываем уведомление в момент мониторинга
    notifyListeners(); 
}

// В деструкторе или методе завершения симуляции:
void Simulator::finalize() {
    for (auto* listener : m_listeners) {
        listener->onSimulationEnd();
    }
}

void Simulator::updateStatistics(int userId, double currentTime) {
    if (userId < 0 || userId >= m_users) return;
    
    double dt = currentTime - m_lastEventTime[userId];
    if (dt <= 0.0) return;
    
    if (m_userStates[userId]) {
        m_stats.totalActiveTime[userId] += dt;
        m_stats.nodeBusyTime += dt;
    } else {
        m_stats.totalPassiveTime[userId] += dt;
    }
    m_lastEventTime[userId] = currentTime;
}

void Simulator::updateGlobalStatistics(double currentTime) {
    if (currentTime <= m_statUpdateTime) return;
    
    int activeCount = std::count(m_userStates.begin(), m_userStates.end(), true);
    double dt = currentTime - m_statUpdateTime;
    
    if (activeCount >= 0 && activeCount <= m_users) {
        m_stats.timeInState[activeCount] += dt;
    }
    m_statUpdateTime = currentTime;
}

double Simulator::getTotalWorkload() const {
    double total = 0.0;
    for (int i = 0; i < m_users; ++i) {
        if (m_userStates[i] && m_Workload[i] > 0.0) {
            total += m_Workload[i];
        }
    }
    return total;
}

double Simulator::computeEffectiveRate(double totalWorkload) const {
    return m_baseServiceRate * m_degradationFn(totalWorkload);
}

void Simulator::rescaleRemainingTimes(double oldRate, double newRate, int excludingUserId) {
    if (std::abs(oldRate - newRate) < 1e-12 || newRate <= 0.0) return;
    
    double ratio = oldRate / newRate;
    
    for (int uid = 0; uid < m_users; ++uid) {
        if (uid == excludingUserId) continue;
        if (uid < 0 || uid >= m_users) continue;
        if (!m_userStates[uid] || m_remainingTime[uid] <= 0.0) continue;
        
        m_remainingTime[uid] = std::max(0.0, m_remainingTime[uid] * ratio);
    }
}