#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "EventQueue.h"
#include "Distribution.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <fstream>

struct SimulationStats {
    // Для каждого пользователя
    std::vector<double> totalActiveTime;   // Σ время в активной фазе
    std::vector<double> totalPassiveTime;  // Σ время в пассивной фазе
    std::vector<int> taskCount;            // Число завершённых задач
    
    // Для узла в целом
    double nodeBusyTime = 0.0;             // Интеграл от числа активных пользователей по времени
    int maxConcurrentUsers = 0;            // Пик одновременных активных пользователей
    int totalEventsProcessed = 0;          // Общее число обработанных событий
    double totalSimulationTime = 0.0;      // Фактическое время симуляции

    std::vector<double> timeInState;  // timeInState[k] = время с ровно k активными

    // Новые поля для учета ресурсов
    double totalResourceConsumption = 0.0;  // Интеграл от суммарного ресурсопотребления
    std::vector<double> resourceRequirements;  // Требуемый ресурс для каждого пользователя
    std::vector<double> timeByTotalResource;  // timeByTotalResource[r] = время с суммарным ресурсопотреблением ~r

    // Конструктор с инициализацией векторов
    explicit SimulationStats(int numUsers, const std::vector<double>& resourceReqs)
        : totalActiveTime(numUsers, 0.0),
          totalPassiveTime(numUsers, 0.0),
          taskCount(numUsers, 0),
          timeInState(numUsers + 1, 0.0),
          resourceRequirements(resourceReqs),
          timeByTotalResource(numUsers * 10 + 1, 0.0) {}  // Предполагаем, что максимальный суммарный ресурс ~numUsers*avg_resource

    std::vector<double> getProbabilityDistribution() const {
        std::vector<double> pk(timeInState.size(), 0.0);
        if (totalSimulationTime <= 0.0) return pk;
        
        for (size_t k = 0; k < timeInState.size(); ++k) {
            pk[k] = timeInState[k] / totalSimulationTime;
        }
        return pk;
    }

    // Новая функция для получения распределения по суммарному ресурсопотреблению
    std::vector<double> getResourceDistribution() const {
        std::vector<double> pr(timeByTotalResource.size(), 0.0);
        if (totalSimulationTime <= 0.0) return pr;
        
        for (size_t r = 0; r < timeByTotalResource.size(); ++r) {
            pr[r] = timeByTotalResource[r] / totalSimulationTime;
        }
        return pr;
    }

    // Расчётные метрики
    double getNodeUtilization(int totalUsers) const {
        return totalSimulationTime > 0 
            ? nodeBusyTime / (totalSimulationTime * totalUsers) 
            : 0.0;
    }

    double getAvgResourceUtilization() const {
        return totalSimulationTime > 0 
            ? totalResourceConsumption / totalSimulationTime 
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
        double avgResourceUtil = getAvgResourceUtilization();

        auto pk = getProbabilityDistribution();
        auto pr = getResourceDistribution();
        
        std::cout << "\n=== Результаты симуляции ===\n";
        std::cout << "Время симуляции:        " << std::fixed << std::setprecision(2) 
                  << totalSimulationTime << " сек\n";
        std::cout << "Число пользователей:    " << totalUsers << "\n";
        std::cout << "Загрузка узла (ρ):      " << std::fixed << std::setprecision(4) 
                  << utilization << " (" << utilization * 100 << "%)\n";
        std::cout << "Средняя загрузка ресурса: " << std::fixed << std::setprecision(4) 
                  << avgResourceUtil << "\n";
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
            if (pk[k] > 0.0001) {  // Печатаем только значимые значения
                std::cout << std::setw(2) << k << " | " 
                            << std::fixed << std::setprecision(4) << pk[k] << " | ";
                int bars = static_cast<int>(pk[k] * 50);
                for (int i = 0; i < bars; ++i) std::cout << "█";
                std::cout << "\n";
            }
        }
        std::cout << "============================\n";

        std::cout << "\nРаспределение суммарного ресурсопотребления P(R):\n";
        std::cout << " R |  P(R)   | Гистограмма\n";
        std::cout << "---|---------|------------\n";
        int printed = 0;
        for (size_t r = 0; r < pr.size() && printed < 20; ++r) {  // Ограничиваем вывод
            if (pr[r] > 0.0001) {  // Печатаем только значимые значения
                std::cout << std::setw(2) << r << " | " 
                            << std::fixed << std::setprecision(4) << pr[r] << " | ";
                int bars = static_cast<int>(pr[r] * 50);
                for (int i = 0; i < bars; ++i) std::cout << "█";
                std::cout << "\n";
                printed++;
            }
        }
        std::cout << "============================\n";
    }
};

class Simulator {
private:
    EventQueue eventQueue_;
    double currentTime_ = 0.0;
    const int maxUsers_;  // Ограничение числа пользователей (закрытая система)
    double lastGlobalEventTime_ = 0.0;
    void updateGlobalStatistics(double currentTime);
    
    // Распределения для фаз
    std::unique_ptr<Distribution> activeTimeDist_;   // время работы (активная фаза)
    std::unique_ptr<Distribution> passiveTimeDist_;  // время простоя (пассивная фаза)
    
    // Новое: распределение для требования ресурса
    std::unique_ptr<Distribution> resourceRequirementDist_;
    
    // Состояние пользователей
    std::vector<bool> userStates_;      // true = активен, false = пассивен
    std::vector<double> lastEventTime_; // время последнего события для каждого пользователя
    std::vector<double> resourceRequirements_; // требуемый ресурс для каждого пользователя
    
    // Состояние ресурсов системы
    double currentResourceConsumption_ = 0.0; // текущее суммарное потребление ресурсов
    double maxResourceCapacity_ = 100.0;      // максимальная емкость ресурса системы (можно настроить)
    
    SimulationStats stats_;
    
    // Инкрементальное обновление статистики ПЕРЕД изменением состояния
    void updateStatistics(int userId, double currentTime);
    
    // Вычисление коэффициента деградации системы
    double calculateDegradationFactor() const;
    
    // Корректировка времени события с учетом деградации
    double adjustTimeForDegradation(double originalTime, double resourceConsumption) const;
    
    // Обработчики событий
    void handleActivation(int userId);
    void handleDeactivation(int userId);
    void handleMonitoring();
    
    // Планирование периодического мониторинга
    void scheduleMonitoring(double nextTime);

public:
    Simulator(
        int maxUsers,
        std::unique_ptr<Distribution> activeDist,
        std::unique_ptr<Distribution> passiveDist,
        std::unique_ptr<Distribution> resourceDist,
        double maxCapacity = 100.0
    );
    
    // Инициализация: все пользователи начинают в пассивной фазе
    void initialize();
    
    // Запуск симуляции до заданного времени
    void runUntil(double endTime);
    
    // Текущее время симуляции
    double currentTime() const { return currentTime_; }
    
    // Получение статистики
    const SimulationStats& getStats() const { return stats_; }
};

#endif // SIMULATOR_H