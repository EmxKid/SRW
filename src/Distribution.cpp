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

// Детерминированное (фиксированное) распределение
class DeterministicDist : public Distribution {
    double value_;
public:
    DeterministicDist(double value) : value_(value) {}
    double sample() override { return value_; }
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
    double sample() override { return randNormal(mean_, stddev_); }
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
    double sample() override { return randGamma(shape_, scale_); }
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
    double sample() override { return randLognormal(mu_, sigma_); }
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