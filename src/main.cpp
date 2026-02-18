#include "Simulator.h"
#include "Distribution.h"
#include "RandomGenerator.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>

int main() {
    // 1. Фиксируем сид для воспроизводимости
    RandomGenerator::instance().setSeed(56);
    
    // 2. Настройка параметров системы
    const int NUM_USERS = 10;
    const double SIM_TIME = 1000000.0;  // 10000 секунд модельного времени
    
    // 3. Выбор распределений для фаз
    // Вариант А: экспоненциальные времена (классическая СМО)
    //   - Активная фаза: среднее время = 2.0 сек (интенсивность μ = 0.5)
    //   - Пассивная фаза: среднее время = 3.0 сек (интенсивность λ = 1/3)
    auto activeDist = DistributionFactory::exponential(0.5);   // μ = 0.5 → E[T] = 2.0
    auto passiveDist = DistributionFactory::exponential(1.0/3.0); // λ = 1/3 → E[T] = 3.0
    
    // Вариант Б: более реалистичные распределения (раскомментировать для теста)
    // auto activeDist = DistributionFactory::erlang(3, 1.5);   // k=3, μ=1.5 → E[T]=2.0, низкая вариация
    // auto passiveDist = DistributionFactory::hyperexponential(0.7, 0.2, 2.0); // смесь лёгких/тяжёлых пауз
    
    // 4. Создание симулятора
    Simulator sim(NUM_USERS, std::move(activeDist), std::move(passiveDist));
    
    // 5. Запуск симуляции
    std::cout << "Запуск симуляции: " << NUM_USERS << " пользователей, " 
              << SIM_TIME << " секунд...\n";
    sim.runUntil(SIM_TIME);
    
    // 6. Вывод результатов
    sim.getStats().printSummary(NUM_USERS);
    
    // 7. Дополнительный анализ: сравнение с теоретической моделью закрытой СМО
    // Для экспоненциальных времён можно сравнить с моделью Машина-Робинсона
    auto pk = sim.getStats().getProbabilityDistribution();
    double theoretical_rho = 0.0;
    
    // Простая оценка загрузки: ρ = (среднее активное время) / (среднее активное + пассивное)
    double avgActive = sim.getStats().getAvgActiveTime();
    double avgPassive = sim.getStats().getAvgPassiveTime();
    theoretical_rho = avgActive / (avgActive + avgPassive);
    
    std::cout << "\n=== Теоретическая оценка ===\n";
    std::cout << "Среднее активное время:   " << std::fixed << std::setprecision(3) << avgActive << " сек\n";
    std::cout << "Среднее пассивное время:  " << std::fixed << std::setprecision(3) << avgPassive << " сек\n";
    std::cout << "Теоретическая загрузка ρ: " << std::fixed << std::setprecision(4) << theoretical_rho << "\n";
    std::cout << "Эмпирическая загрузка ρ:  " << std::fixed << std::setprecision(4) 
              << sim.getStats().getNodeUtilization(NUM_USERS) << "\n";
    
    // 8. Сохранение распределения P(k) в CSV для анализа в Python/R
    std::ofstream csv("pk_distribution.csv");
    if (csv.is_open()) {
        csv << "k,P(k)\n";
        for (size_t k = 0; k < pk.size(); ++k) {
            csv << k << "," << pk[k] << "\n";
        }
        csv.close();
        std::cout << "\nРаспределение P(k) сохранено в pk_distribution.csv\n";
    }
    
    return 0;
}