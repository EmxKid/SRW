#include "RandomGenerator.h"
#include <stdexcept>
#include <random>

RandomGenerator::RandomGenerator() 
    : seed_(std::random_device{}()), gen_(seed_) {}

double RandomGenerator::uniform(double a, double b) {
    std::uniform_real_distribution<double> dist(a, b);
    return dist(gen_);
}

double RandomGenerator::exponential(double rate) {
    if (rate <= 0.0) throw std::invalid_argument("Rate must be > 0");
    std::exponential_distribution<double> dist(rate);
    return dist(gen_);
}

int RandomGenerator::integer(int min, int max) {
    if (min > max) throw std::invalid_argument("Invalid range");
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen_);
}

void RandomGenerator::setSeed(unsigned int seed) {
    seed_ = seed;
    gen_.seed(seed);
}

unsigned int RandomGenerator::getSeed() const {
    return seed_;  // Возвращаем сохранённое семя, а не пытаемся извлечь из генератора
}

std::string RandomGenerator::getState() const {
    std::ostringstream oss;
    oss << gen_;  // Сериализация полного состояния генератора
    return oss.str();
}

void RandomGenerator::setState(const std::string& state) {
    std::istringstream iss(state);
    iss >> gen_;  // Десериализация
}