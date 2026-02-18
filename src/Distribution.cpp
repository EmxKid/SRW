// src/Distribution.cpp
#include "Distribution.h"
#include "RandomGenerator.h"
#include <cmath>
#include <stdexcept>
#include <memory>

// Экспоненциальное распределение
class ExponentialDist : public Distribution {
    double rate_;
public:
    ExponentialDist(double rate) : rate_(rate) {
        if (rate <= 0) throw std::invalid_argument("Rate > 0 required");
    }
    double sample() override { return randExponential(rate_); }
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