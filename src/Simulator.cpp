#include "Simulator.h"
#include "RandomGenerator.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>

Simulator::Simulator(
    int maxUsers,
    std::unique_ptr<Distribution> activeDist,
    std::unique_ptr<Distribution> passiveDist,
    std::unique_ptr<Distribution> resourceDist
) : maxUsers_(maxUsers),
    activeTimeDist_(std::move(activeDist)),
    passiveTimeDist_(std::move(passiveDist)),
    resourceRequirementDist_(std::move(resourceDist)),
    userStates_(maxUsers_, false),      // все начинают в пассивной фазе
    lastEventTime_(maxUsers_, 0.0),     // время последнего события = 0 для всех
    resourceRequirements_(maxUsers_),
    stats_(maxUsers_, std::vector<double>(maxUsers_, 0.0))  // временный вектор, будет переназначен позже
{
    if (maxUsers_ <= 0)
        throw std::invalid_argument("Число пользователей должно быть > 0");
    if (!activeTimeDist_ || !passiveTimeDist_ || !resourceRequirementDist_)
        throw std::invalid_argument("Распределения не должны быть nullptr");
    
    // Инициализация требований ресурса для каждого пользователя
    for (int i = 0; i < maxUsers_; ++i) {
        resourceRequirements_[i] = resourceRequirementDist_->sample();
    }
    
    // Пересоздаем статистику с правильным вектором требований ресурса
    stats_ = SimulationStats(maxUsers_, resourceRequirements_);
}

void Simulator::initialize() {
    // Все пользователи начинают в пассивной фазе
    for (int userId = 0; userId < maxUsers_; ++userId) {
        double nextActivation = currentTime_ + passiveTimeDist_->sample();
        eventQueue_.push(Event{
            nextActivation,
            EventType::ACTIVATION,
            userId,
            [this, userId]() { handleActivation(userId); }
        });
        lastEventTime_[userId] = currentTime_;
    }
    
    // Периодический мониторинг каждую секунду
    scheduleMonitoring(currentTime_ + 1.0);
}

void Simulator::scheduleMonitoring(double nextTime) {
    if (nextTime <= currentTime_) return;
    
    eventQueue_.push(Event{
        nextTime,
        EventType::MONITORING,
        -1,  // системное событие
        [this, nextTime]() {
            handleMonitoring();
            scheduleMonitoring(nextTime + 1.0);
        }
    });
}

void Simulator::updateStatistics(int userId, double currentTime) {
    if (userId < 0 || userId >= maxUsers_)
        return;  // игнорируем системные события
    
    double dt = currentTime - lastEventTime_[userId];
    if (dt <= 0.0) return;
    
    if (userStates_[userId]) {
        // Пользователь был АКТИВЕН → учитываем в загрузке узла
        stats_.totalActiveTime[userId] += dt;
        stats_.nodeBusyTime += dt;  // ← КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: обновление загрузки узла
    } else {
        // Пользователь был ПАССИВЕН
        stats_.totalPassiveTime[userId] += dt;
    }
    
    lastEventTime_[userId] = currentTime;
}

void Simulator::handleActivation(int userId) {
    // 1. Обновляем статистику за предыдущий интервал (пассивная фаза)
    updateGlobalStatistics(currentTime_);
    updateStatistics(userId, currentTime_);
    
    // 2. Меняем состояние на активное
    userStates_[userId] = true;
    
    // 3. Планируем завершение задачи
    double taskDuration = activeTimeDist_->sample();
    eventQueue_.push(Event{
        currentTime_ + taskDuration,
        EventType::DEACTIVATION,
        userId,
        [this, userId]() { handleDeactivation(userId); }
    });
    
    // 4. Обновляем пиковую статистику
    int activeCount = std::count(userStates_.begin(), userStates_.end(), true);
    stats_.maxConcurrentUsers = std::max(stats_.maxConcurrentUsers, activeCount);
}

void Simulator::handleDeactivation(int userId) {
    updateGlobalStatistics(currentTime_);
    // 1. Обновляем статистику за предыдущий интервал (активная фаза)
    updateStatistics(userId, currentTime_);
    
    // 2. Меняем состояние на пассивное
    userStates_[userId] = false;
    
    // 3. Увеличиваем счётчик задач
    stats_.taskCount[userId]++;
    
    // 4. Планируем следующее пробуждение
    double nextPassive = passiveTimeDist_->sample();
    eventQueue_.push(Event{
        currentTime_ + nextPassive,
        EventType::ACTIVATION,
        userId,
        [this, userId]() { handleActivation(userId); }
    });
}

void Simulator::handleMonitoring() {
    // Опционально: запись временного ряда или отладочный вывод
    // Например: stats_.timeSeries.push_back({currentTime_, activeCount});
}

void Simulator::runUntil(double endTime) {
    if (endTime <= 0.0)
        throw std::invalid_argument("Время симуляции должно быть > 0");
    
    lastGlobalEventTime_ = 0.0;
    currentTime_ = 0.0;

    initialize();
    
    while (!eventQueue_.empty() && currentTime_ < endTime) {
        Event event = eventQueue_.pop();
        currentTime_ = event.time;
        event.handler();
        stats_.totalEventsProcessed++;
    }
    
    // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: учитываем ВЕСЬ интервал до endTime, а не до последнего события
    for (int userId = 0; userId < maxUsers_; ++userId) {
        updateStatistics(userId, endTime);  // ← endTime, а не currentTime_!
    }
    
    updateGlobalStatistics(endTime);
    stats_.totalSimulationTime = endTime;
}

void Simulator::updateGlobalStatistics(double currentTime) {
    if (currentTime <= lastGlobalEventTime_) return;
    
    int activeCount = std::count(userStates_.begin(), userStates_.end(), true);
    double dt = currentTime - lastGlobalEventTime_;
    
    // Обновляем статистику по числу активных пользователей
    if (activeCount >= 0 && activeCount <= maxUsers_) {
        stats_.timeInState[activeCount] += dt;
    }
    
    // НОВОЕ: Обновляем статистику по суммарному ресурсопотреблению
    double totalResource = 0.0;
    for (int i = 0; i < maxUsers_; ++i) {
        if (userStates_[i]) {  // Если пользователь активен
            totalResource += resourceRequirements_[i];
        }
    }
    
    // Интегрируем суммарное ресурсопотребление
    stats_.totalResourceConsumption += totalResource * dt;
    
    // Обновляем время, проведенное с данным уровнем ресурсопотребления
    // Индексируем по целой части ресурса (можно адаптировать под другую дискретизацию)
    int resourceIndex = static_cast<int>(totalResource);
    if (resourceIndex >= 0 && resourceIndex < static_cast<int>(stats_.timeByTotalResource.size())) {
        stats_.timeByTotalResource[resourceIndex] += dt;
    }
    
    lastGlobalEventTime_ = currentTime;
}