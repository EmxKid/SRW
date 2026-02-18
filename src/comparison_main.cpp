#include "Simulator.h"
#include "Distribution.h"
#include "RandomGenerator.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <vector>

int main() {
    // 1. Фиксируем сид для воспроизводимости
    RandomGenerator::instance().setSeed(56);
    
    // 2. Настройка параметров системы
    const int NUM_USERS = 10;
    const double SIM_TIME = 1000000.0;  // 10000 секунд модельного времени
    
    std::cout << "=== Сравнение различных распределений длительности активной фазы ===\n\n";
    
    // Вариант А: экспоненциальные времена (классическая СМО)
    std::cout << "Вариант А: Оба экспоненциальные (базовый случай)\n";
    {
        auto activeDist = DistributionFactory::exponential(0.5);   // μ = 0.5 → E[T] = 2.0
        auto passiveDist = DistributionFactory::exponential(1.0/3.0); // λ = 1/3 → E[T] = 3.0
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist));
        sim.runUntil(SIM_TIME);
        
        auto stats = sim.getStats();
        std::cout << "  Загрузка узла: " << std::fixed << std::setprecision(4) 
                  << stats.getNodeUtilization(NUM_USERS) << "\n";
                auto pk = stats.getProbabilityDistribution();
        std::cout << "  Распределение P(k): ";
        for (size_t i = 0; i < std::min(pk.size(), (size_t)5); ++i) {
            std::cout << "[" << i << ":" << std::fixed << std::setprecision(3) << pk[i] << "] ";
        }
        std::cout << "\n\n";
    }
    
    // Вариант B: нормальное распределение для активной фазы
    std::cout << "Вариант B: Нормальное для активной фазы, экспоненциальное для пассивной\n";
    {
        auto activeDist = DistributionFactory::normal(2.0, 0.5);   // E[T] = 2.0, stddev = 0.5
        auto passiveDist = DistributionFactory::exponential(1.0/3.0); // λ = 1/3 → E[T] = 3.0
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist));
        sim.runUntil(SIM_TIME);
        
        auto stats = sim.getStats();
        std::cout << "  Загрузка узла: " << std::fixed << std::setprecision(4) 
                  << stats.getNodeUtilization(NUM_USERS) << "\n";
        auto pk = stats.getProbabilityDistribution();
        std::cout << "  Распределение P(k): ";
        for (size_t i = 0; i < std::min(pk.size(), (size_t)5); ++i) {
            std::cout << "[" << i << ":" << std::fixed << std::setprecision(3) << pk[i] << "] ";
        }
        std::cout << "\n\n";
    }
    
    // Вариант C: гамма-распределение для активной фазы
    std::cout << "Вариант C: Гамма для активной фазы, экспоненциальное для пассивной\n";
    {
        auto activeDist = DistributionFactory::gamma(2.0, 1.0);   // shape=2, scale=1 → E[T] = 2.0
        auto passiveDist = DistributionFactory::exponential(1.0/3.0); // λ = 1/3 → E[T] = 3.0
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist));
        sim.runUntil(SIM_TIME);
        
        auto stats = sim.getStats();
        std::cout << "  Загрузка узла: " << std::fixed << std::setprecision(4) 
                  << stats.getNodeUtilization(NUM_USERS) << "\n";
                auto pk = stats.getProbabilityDistribution();
        std::cout << "  Распределение P(k): ";
        for (size_t i = 0; i < std::min(pk.size(), (size_t)5); ++i) {
            std::cout << "[" << i << ":" << std::fixed << std::setprecision(3) << pk[i] << "] ";
        }
        std::cout << "\n\n";
    }
    // Вариант D: логнормальное распределение для активной фазы
    std::cout << "Вариант D: Логнормальное для активной фазы, экспоненциальное для пассивной\n";
    {
        auto activeDist = DistributionFactory::lognormal(0.6, 0.4);  // E[T] ≈ 2.0
        auto passiveDist = DistributionFactory::exponential(1.0/3.0); // λ = 1/3 → E[T] = 3.0
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist));
        sim.runUntil(SIM_TIME);
        
        auto stats = sim.getStats();
        std::cout << "  Загрузка узла: " << std::fixed << std::setprecision(4) 
                  << stats.getNodeUtilization(NUM_USERS) << "\n";
        auto pk = stats.getProbabilityDistribution();
        std::cout << "  Распределение P(k): ";
        for (size_t i = 0; i < std::min(pk.size(), (size_t)5); ++i) {
            std::cout << "[" << i << ":" << std::fixed << std::setprecision(3) << pk[i] << "] ";
        }
        std::cout << "\n\n";
    }
    
    // Вариант E: детерминированное распределение для пассивной фазы
    std::cout << "Вариант E: Экспоненциальное для активной, детерминированное для пассивной\n";
    {
        auto activeDist = DistributionFactory::exponential(0.5);   // μ = 0.5 → E[T] = 2.0
        auto passiveDist = DistributionFactory::deterministic(3.0); // E[T] = 3.0, детерминировано
        
        Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist));
        sim.runUntil(SIM_TIME);
        
        auto stats = sim.getStats();
        std::cout << "  Загрузка узла: " << std::fixed << std::setprecision(4) 
                  << stats.getNodeUtilization(NUM_USERS) << "\n";
        auto pk = stats.getProbabilityDistribution();
        std::cout << "  Распределение P(k): ";
        for (size_t i = 0; i < std::min(pk.size(), (size_t)5); ++i) {
            std::cout << "[" << i << ":" << std::fixed << std::setprecision(3) << pk[i] << "] ";
        }
        std::cout << "\n\n";
    }
    
    return 0;
}