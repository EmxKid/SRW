#ifndef RANDOM_GENERATOR_H
#define RANDOM_GENERATOR_H

#include <random>
#include <string>
#include <sstream>

class RandomGenerator {
public:
    // Единая точка доступа (Singleton через статический метод)
    static RandomGenerator& instance() {
        static RandomGenerator instance;
        return instance;
    }

    // Запрещаем копирование
    RandomGenerator(const RandomGenerator&) = delete;
    RandomGenerator& operator=(const RandomGenerator&) = delete;

    // Генерация случайных величин
    double uniform(double a = 0.0, double b = 1.0);
    double exponential(double rate);  // rate = λ, среднее = 1/λ
    int    integer(int min, int max); // включительно [min, max]

    // Установка/получение семени для воспроизводимости
    void setSeed(unsigned int seed);
    unsigned int getSeed() const;  // Возвращает ИСХОДНОЕ семя

    // Полная сериализация состояния (для воспроизводимости промежуточных состояний)
    std::string getState() const;
    void setState(const std::string& state);

    // Публичный доступ к генератору для новых распределений
    std::mt19937& generator() { return gen_; }

private:
    RandomGenerator(); // приватный конструктор
    std::mt19937 gen_;
    unsigned int seed_;  // Явно сохраняем семя
};

// Глобальные удобные псевдонимы
inline double randUniform(double a = 0.0, double b = 1.0) {
    return RandomGenerator::instance().uniform(a, b);
}

inline double randExponential(double rate) {
    return RandomGenerator::instance().exponential(rate);
}

inline double randNormal(double mean, double stddev) {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(RandomGenerator::instance().generator());
}

inline double randGamma(double shape, double scale) {
    std::gamma_distribution<double> dist(shape, scale);
    return dist(RandomGenerator::instance().generator());
}

inline double randLognormal(double mu, double sigma) {
    std::lognormal_distribution<double> dist(mu, sigma);
    return dist(RandomGenerator::instance().generator());
}

#endif // RANDOM_GENERATOR_H