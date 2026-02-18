// include/Distribution.h
#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include <memory>
#include <string>
#include <functional>

class Distribution {
public:
    virtual ~Distribution() = default;
    
    // Генерация случайной величины
    virtual double sample() = 0;
    
    // Среднее значение (для аналитических расчётов)
    virtual double mean() const = 0;
    
    // Имя распределения (для логгирования/отладки)
    virtual std::string name() const = 0;
    
    // Клонирование (для безопасного копирования)
    virtual std::unique_ptr<Distribution> clone() const = 0;
};

// Фабрика распределений
class DistributionFactory {
public:
    // Экспоненциальное: f(t) = λ·e^(-λt)
    static std::unique_ptr<Distribution> exponential(double rate);
    
    // Детерминированное (фиксированное время)
    static std::unique_ptr<Distribution> deterministic(double value);
    
};

#endif // DISTRIBUTION_H