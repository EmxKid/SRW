#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "EventQueue.h"
#include "Distribution.h"
#include "ISimulationListener.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <map>
#include <cstdint>

struct SimulationStats {
    std::vector<double> totalActiveTime;
    std::vector<double> totalPassiveTime;
    std::vector<int> taskCount;
    
    double nodeBusyTime = 0.0;
    int maxConcurrentUsers = 0;
    int totalEventsProcessed = 0;
    double totalSimulationTime = 0.0;

    std::vector<double> totalWorkCompleted;
    double totalWorkProcessed = 0.0;
    
    std::map<int, double> completionTimeHistogram;
    
    double avgDegradationFactor = 1.0;
    int degradationSamples = 0;

    std::vector<double> timeInState;

    explicit SimulationStats(int numUsers)
        : totalActiveTime(numUsers, 0.0),
          totalPassiveTime(numUsers, 0.0),
          taskCount(numUsers, 0),
          totalWorkCompleted(numUsers, 0.0),
          timeInState(numUsers + 1, 0.0) {} 

    std::vector<double> getProbabilityDistribution() const {
        std::vector<double> pk(timeInState.size(), 0.0);
        if (totalSimulationTime <= 0.0) return pk;
        for (size_t k = 0; k < timeInState.size(); ++k) {
            pk[k] = timeInState[k] / totalSimulationTime;
        }
        return pk;
    }

    void recordDegradation(double factor) {
        avgDegradationFactor = (avgDegradationFactor * degradationSamples + factor) 
                              / (degradationSamples + 1);
        degradationSamples++;
    }

    double getNodeUtilization(int totalUsers) const {
        return totalSimulationTime > 0 
            ? nodeBusyTime / (totalSimulationTime * totalUsers) 
            : 0.0;
    }

    double getAvgActiveTime() const {
        return totalActiveTime.empty() ? 0.0 
            : std::accumulate(totalActiveTime.begin(), totalActiveTime.end(), 0.0) 
              / totalActiveTime.size();
    }

    double getAvgPassiveTime() const {
        return totalPassiveTime.empty() ? 0.0 
            : std::accumulate(totalPassiveTime.begin(), totalPassiveTime.end(), 0.0) 
              / totalPassiveTime.size();
    }

    double getAvgTaskCount() const {
        return taskCount.empty() ? 0.0 
            : std::accumulate(taskCount.begin(), taskCount.end(), 0) 
              / static_cast<double>(taskCount.size());
    }

    void printSummary(int totalUsers) const {
        double utilization = getNodeUtilization(totalUsers);
        auto pk = getProbabilityDistribution();
        
        std::cout << "\n=== Результаты симуляции ===\n";
        std::cout << "Время симуляции:        " << std::fixed << std::setprecision(2) 
                  << totalSimulationTime << " сек\n";
        std::cout << "Число пользователей:    " << totalUsers << "\n";
        std::cout << "Загрузка узла (ρ):      " << std::fixed << std::setprecision(4) 
                  << utilization << " (" << utilization * 100 << "%)\n";
        std::cout << "Максимум активных:      " << maxConcurrentUsers << " / " << totalUsers << "\n";
        std::cout << "Среднее время активности: " << std::fixed << std::setprecision(3) 
                  << getAvgActiveTime() << " сек\n";
        std::cout << "Среднее время простоя:  " << std::fixed << std::setprecision(3) 
                  << getAvgPassiveTime() << " сек\n";
        std::cout << "Среднее число задач:    " << std::fixed << std::setprecision(1) 
                  << getAvgTaskCount() << "\n";
        std::cout << "Обработано событий:     " << totalEventsProcessed << "\n";
        std::cout << "============================\n";
        
        std::cout << "\nРаспределение числа активных пользователей P(k):\n";
        std::cout << " k |   P(k)   | Гистограмма\n";
        std::cout << "---|----------|------------\n";
        for (size_t k = 0; k < pk.size(); ++k) {
            std::cout << std::setw(2) << k << " | " 
                        << std::fixed << std::setprecision(4) << pk[k] << " | ";
            int bars = static_cast<int>(pk[k] * 50);
            for (int i = 0; i < bars; ++i) std::cout << "█";
            std::cout << "\n";
        }
        std::cout << "============================\n";
    }
};

class Simulator {
private:
    const int m_users;
    
    double m_currentTime = 0.0;
    double m_statUpdateTime = 0.0;
    double m_baseServiceRate;
    double m_currentEffectiveRate;
    
    std::unique_ptr<Distribution> m_workloadDist;
    std::unique_ptr<Distribution> m_passiveTimeDist;
    std::unique_ptr<Distribution> m_serviceTime;
    std::function<double(double)> m_degradationFn;
    
    std::vector<bool> m_userStates;
    std::vector<double> m_lastEventTime;
    std::vector<double> m_Workload;
    std::vector<double> m_remainingTime;
    std::vector<uint64_t> m_eventVersion;
    
    SimulationStats m_stats;
    EventQueue m_eventQueue;

    void updateGlobalStatistics(double currentTime);
    void updateStatistics(int userId, double currentTime);
    void handleActivation(int userId);
    void handleDeactivation(int userId);
    void handleMonitoring();
    void scheduleMonitoring(double nextTime);

    std::vector<ISimulationListener*> m_listeners;
    void notifyListeners();
    
    double getTotalWorkload() const;
    double computeEffectiveRate(double totalWorkload) const;
    void rescaleRemainingTimes(double oldRate, double newRate, int excludingUserId = -1);
    
public:
    Simulator(
        int maxUsers,
        double baseServiceRate,
        std::unique_ptr<Distribution> workloadDist,
        std::unique_ptr<Distribution> passiveTimeDist,
        std::unique_ptr<Distribution> serviceTimeDist,
        std::function<double(double)> degradationFn
    );
    
    void initialize();
    void runUntil(double endTime);
    double currentTime() const { return m_currentTime; }
    const SimulationStats& getStats() const { return m_stats; }
    void attachListener(ISimulationListener* listener);
    void finalize();
};

#endif