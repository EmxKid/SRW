// src/Distribution.cpp
#include "Distribution.h"
#include "RandomGenerator.h"
#include <cmath>
#include <stdexcept>
#include <memory>
#include <iostream>

// Экспоненциальное распределение
class ExponentialDist : public Distribution {
    double rate_;
public:
    ExponentialDist(double rate) : rate_(rate) {
        if (rate <= 0) throw std::invalid_argument("Rate > 0 required");
    }
    double sample(std::optional<double> rate) override { 
        double actualRate = rate.value_or(rate_);
        return randExponential(actualRate); 
    }
    double mean() const override { return 1.0 / rate_; }
    std::string name() const override { return "Exp(λ=" + std::to_string(rate_) + ")"; }
    std::unique_ptr<Distribution> clone() const override {
        return std::make_unique<ExponentialDist>(rate_);
    }
};


// Реализация фабрики
std::unique_ptr<Distribution> DistributionFactory::exponential(double rate) {
    return std::make_unique<ExponentialDist>(rate);
}

// Детерминированное (фиксированное) распределение
class DeterministicDist : public Distribution {
    double value_;
public:
    DeterministicDist(double value) : value_(value) {}
    double sample(std::optional<double> rate) override { 
        return value_; 
    }
    double mean() const override { return value_; }
    std::string name() const override { return "Det(" + std::to_string(value_) + ")"; }
    std::unique_ptr<Distribution> clone() const override {
        return std::make_unique<DeterministicDist>(value_);
    }
};

std::unique_ptr<Distribution> DistributionFactory::deterministic(double value) {
    return std::make_unique<DeterministicDist>(value);
}

// Нормальное распределение
class NormalDist : public Distribution {
    double mean_, stddev_;
public:
    NormalDist(double mean, double stddev) : mean_(mean), stddev_(stddev) {
        if (stddev <= 0) throw std::invalid_argument("Stddev > 0 required");
    }
    double sample(std::optional<double> rate) override { 
        if (rate.has_value() && rate.value() > 0) {
            // Масштабируем stddev обратно пропорционально скорости:
            // чем выше rate, тем "уже" распределение
            double scaledStddev = stddev_ / rate.value();
            // mean_ при этом можно оставить как есть или тоже масштабировать
            return randNormal(mean_, scaledStddev);
        }
        return randNormal(mean_, stddev_); 
    }
    double mean() const override { return mean_; }
    std::string name() const override { 
        return "N(μ=" + std::to_string(mean_) + ",σ²=" + std::to_string(stddev_*stddev_) + ")"; 
    }
    std::unique_ptr<Distribution> clone() const override {
        return std::make_unique<NormalDist>(mean_, stddev_);
    }
};

std::unique_ptr<Distribution> DistributionFactory::normal(double mean, double stddev) {
    return std::make_unique<NormalDist>(mean, stddev);
}

// Гамма-распределение
class GammaDist : public Distribution {
    double shape_, scale_;
public:
    GammaDist(double shape, double scale) : shape_(shape), scale_(scale) {
        if (shape <= 0 || scale <= 0) throw std::invalid_argument("Shape and scale > 0 required");
    }
    double sample(std::optional<double> rate) override { 
        if (rate.has_value()) {
            // Сохраняем форму (shape), но масштабируем время через scale
            // Новый scale = 1 / rate, тогда mean = shape * (1/rate)
            return randGamma(shape_, 1.0 / rate.value());
        }
        return randGamma(shape_, scale_); 
    }
    double mean() const override { return shape_ * scale_; }
    std::string name() const override { 
        return "Γ(shape=" + std::to_string(shape_) + ",scale=" + std::to_string(scale_) + ")"; 
    }
    std::unique_ptr<Distribution> clone() const override {
        return std::make_unique<GammaDist>(shape_, scale_);
    }
};

std::unique_ptr<Distribution> DistributionFactory::gamma(double shape, double scale) {
    return std::make_unique<GammaDist>(shape, scale);
}

// Логнормальное распределение
class LognormalDist : public Distribution {
    double mu_, sigma_;
public:
    LognormalDist(double mu, double sigma) : mu_(mu), sigma_(sigma) {
        if (sigma <= 0) throw std::invalid_argument("Sigma > 0 required");
    }
    double sample(std::optional<double> rate) override { 
        if (rate.has_value()) {
            // Фиксируем "форму" (sigma), подбираем mu под нужное среднее
            double newMu = std::log(1.0 / rate.value()) - 0.5 * sigma_ * sigma_;
            return randLognormal(newMu, sigma_);
        }
        return randLognormal(mu_, sigma_); 
    }
    double mean() const override { 
        return std::exp(mu_ + 0.5 * sigma_ * sigma_);  // E[X] = exp(μ + σ²/2)
    }
    std::string name() const override { 
        return "LogN(μ=" + std::to_string(mu_) + ",σ²=" + std::to_string(sigma_*sigma_) + ")"; 
    }
    std::unique_ptr<Distribution> clone() const override {
        return std::make_unique<LognormalDist>(mu_, sigma_);
    }
};

std::unique_ptr<Distribution> DistributionFactory::lognormal(double mu, double sigma) {
    return std::make_unique<LognormalDist>(mu, sigma);
}


// Равномерное распределение на отрезке [min, max]
class UniformDist : public Distribution {  // ← не забудьте public наследование!
    double min_, max_;
public:
    UniformDist(double min, double max) : min_(min), max_(max) {
        if (min_ >= max_) throw std::invalid_argument("Uniform: min < max required");
    }
    
    double sample(std::optional<double> rate) override { 
        return randUniform(min_, max_); 
    }
    
    double mean() const override { 
        return (min_ + max_) / 2.0;  // E[X] = (a+b)/2
    }
    
    std::string name() const override { 
        return "U(min=" + std::to_string(min_) + ",max=" + std::to_string(max_) + ")"; 
    }
    
    std::unique_ptr<Distribution> clone() const override {
        return std::make_unique<UniformDist>(min_, max_);
    }
};

std::unique_ptr<Distribution> DistributionFactory::uniform(double min, double max) {
    return std::make_unique<UniformDist>(min, max);
}