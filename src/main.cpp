#include "Simulator.h"
#include "Distribution.h"
#include "RandomGenerator.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>

// Вспомогательная функция для сохранения распределения в CSV
void saveDistributionToCSV(const std::vector<double>& pk, const std::string& filename) {
    std::ofstream csv(filename);
    if (csv.is_open()) {
        csv << "k,P(k)\n";
        for (size_t k = 0; k < pk.size(); ++k) {
            csv << k << "," << pk[k] << "\n";
        }
        csv.close();
        std::cout << "  Распределение P(k) сохранено в " << filename << "\n";
    }
}

// Вспомогательная функция для теоретического анализа
void printTheoreticalAnalysis(const SimulationStats& stats) {
    double avgActive = stats.getAvgActiveTime();
    double avgPassive = stats.getAvgPassiveTime();
    double theoretical_rho = avgActive / (avgActive + avgPassive);
    double empirical_rho = stats.getNodeUtilization(10); // NUM_USERS = 10

    std::cout << "\n  === Теоретическая оценка ===\n";
    std::cout << "  Среднее активное время:   " << std::fixed << std::setprecision(3) << avgActive << " сек\n";
    std::cout << "  Среднее пассивное время:  " << std::fixed << std::setprecision(3) << avgPassive << " сек\n";
    std::cout << "  Теоретическая загрузка ρ: " << std::fixed << std::setprecision(4) << theoretical_rho << "\n";
    std::cout << "  Эмпирическая загрузка ρ:  " << std::fixed << std::setprecision(4) << empirical_rho << "\n";
}

int main() {
    // 1. Фиксируем сид для воспроизводимости
    RandomGenerator::instance().setSeed(56);
    
    // 2. Настройка параметров системы
    const int NUM_USERS = 10;
    const double SIM_TIME = 1000000.0;  // 1 000 000 секунд модельного времени
    
    std::cout << "=== Сравнение различных распределений длительности активной фазы ===\n";
    std::cout << "Параметры системы: " << NUM_USERS << " пользователей, " 
              << SIM_TIME << " секунд симуляции, сид = 56\n\n";

    // Распределение для требования ресурса (пока фиксированное)
    auto resourceDist = DistributionFactory::deterministic(1.0);  // Каждый пользователь требует 1.0 единиц ресурса
    
    // Вариант А: экспоненциальные времена (классическая СМО)
    std::cout << "Вариант А: Оба экспоненциальные (базовый случай)\n";
    std::cout << "  Активная фаза: экспоненциальное (μ=0.5, E[T]=2.0)\n";
    std::cout << "  Пассивная фаза: экспоненциальное (λ=1/3, E[T]=3.0)\n";
    {
        auto activeDist = DistributionFactory::exponential(0.5);
        auto passiveDist = DistributionFactory::exponential(1.0/3.0);
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist), std::move(resourceDist));
        sim.runUntil(SIM_TIME);
        
        // Полный вывод статистики
        sim.getStats().printSummary(NUM_USERS);
        
        // Теоретический анализ
        printTheoreticalAnalysis(sim.getStats());
        
        // Сохранение распределения
        saveDistributionToCSV(sim.getStats().getProbabilityDistribution(), "pk_variant_A.csv");
        std::cout << "\n";
    }
    
    // Вариант B: нормальное распределение для активной фазы
    std::cout << "Вариант B: Нормальное для активной фазы, экспоненциальное для пассивной\n";
    std::cout << "  Активная фаза: нормальное (μ=2.0, σ=0.5)\n";
    std::cout << "  Пассивная фаза: экспоненциальное (λ=1/3, E[T]=3.0)\n";
    {
        auto activeDist = DistributionFactory::normal(2.0, 0.5);
        auto passiveDist = DistributionFactory::exponential(1.0/3.0);
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist), std::move(resourceDist));
        sim.runUntil(SIM_TIME);
        
        sim.getStats().printSummary(NUM_USERS);
        printTheoreticalAnalysis(sim.getStats());
        saveDistributionToCSV(sim.getStats().getProbabilityDistribution(), "pk_variant_B.csv");
        std::cout << "\n";
    }
    
    // Вариант C: гамма-распределение для активной фазы
    std::cout << "Вариант C: Гамма для активной фазы, экспоненциальное для пассивной\n";
    std::cout << "  Активная фаза: гамма (shape=2.0, scale=1.0, E[T]=2.0)\n";
    std::cout << "  Пассивная фаза: экспоненциальное (λ=1/3, E[T]=3.0)\n";
    {
        auto activeDist = DistributionFactory::gamma(2.0, 1.0);
        auto passiveDist = DistributionFactory::exponential(1.0/3.0);
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist), std::move(resourceDist));
        sim.runUntil(SIM_TIME);
        
        sim.getStats().printSummary(NUM_USERS);
        printTheoreticalAnalysis(sim.getStats());
        saveDistributionToCSV(sim.getStats().getProbabilityDistribution(), "pk_variant_C.csv");
        std::cout << "\n";
    }
    
    // Вариант D: логнормальное распределение для активной фазы
    std::cout << "Вариант D: Логнормальное для активной фазы, экспоненциальное для пассивной\n";
    std::cout << "  Активная фаза: логнормальное (μ=0.6, σ=0.4, E[T]≈2.0)\n";
    std::cout << "  Пассивная фаза: экспоненциальное (λ=1/3, E[T]=3.0)\n";
    {
        auto activeDist = DistributionFactory::lognormal(0.6, 0.4);
        auto passiveDist = DistributionFactory::exponential(1.0/3.0);
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist), std::move(resourceDist));
        sim.runUntil(SIM_TIME);
        
        sim.getStats().printSummary(NUM_USERS);
        printTheoreticalAnalysis(sim.getStats());
        saveDistributionToCSV(sim.getStats().getProbabilityDistribution(), "pk_variant_D.csv");
        std::cout << "\n";
    }
    
    // Вариант E: детерминированное распределение для пассивной фазы
    std::cout << "Вариант E: Экспоненциальное для активной, детерминированное для пассивной\n";
    std::cout << "  Активная фаза: экспоненциальное (μ=0.5, E[T]=2.0)\n";
    std::cout << "  Пассивная фаза: детерминированное (всегда 3.0 сек)\n";
    {
        auto activeDist = DistributionFactory::exponential(0.5);
        auto passiveDist = DistributionFactory::deterministic(3.0);
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist), std::move(resourceDist));
        sim.runUntil(SIM_TIME);
        
        sim.getStats().printSummary(NUM_USERS);
        printTheoreticalAnalysis(sim.getStats());
        saveDistributionToCSV(sim.getStats().getProbabilityDistribution(), "pk_variant_E.csv");
        std::cout << "\n";
    }
    
    std::cout << "=== Сравнение завершено ===\n";
    std::cout << "CSV-файлы для анализа в Python/R:\n";
    std::cout << "  pk_variant_A.csv ... pk_variant_E.csv\n";
    
    return 0;
}