// main.cpp
#include "Simulator.h"
#include "Distribution.h"
#include "RandomGenerator.h"
#include "CliUtils.h" 
#include "CsvUtils.h" 
#include "CsvStatisticsCollector.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_map>
#include <optional>
#include <functional>
#include <cmath>

// === Структура аргументов командной строки ===
struct Args {
    int users = 10;                    // число пользователей
    double simTime = 1000.0;           // время симуляции
    int seed = 42;                     // seed для ГСЧ
    std::string workloadDist = "exp:1.0";  // распределение ОБЪЁМА работы (было --active)
    std::string serviceTimeDist = "exp:1.0";
    std::string passiveDist = "exp:0.5";   // распределение времени простоя
    double baseRate = 1.0;             // базовая скорость обслуживания μ₀
    std::string degradationSpec = "hyp:10.0"; // спецификация функции деградации
    std::string csvOutput;             // файл для вывода P(k)
    bool help = false;                 // флаг помощи
};

// === Парсер аргументов командной строки ===
Args parseArgs(int argc, char* argv[]) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            args.help = true;
            
        } else if (arg == "--users" && i+1 < argc) {
            args.users = std::stoi(argv[++i]);
            
        } else if (arg == "--time" && i+1 < argc) {
            args.simTime = std::stod(argv[++i]);
            
        } else if (arg == "--seed" && i+1 < argc) {
            args.seed = std::stoi(argv[++i]);
            
        } else if (arg == "--workload" && i+1 < argc) {  // ← было --active
            args.workloadDist = argv[++i];

        } else if (arg == "--service-time" && i+1 < argc) {
            args.serviceTimeDist = argv[++i];

        } else if (arg == "--passive" && i+1 < argc) {
            args.passiveDist = argv[++i];
            
        } else if (arg == "--base-rate" && i+1 < argc) {  // ← новое
            args.baseRate = std::stod(argv[++i]);
            
        } else if (arg == "--degradation" && i+1 < argc) {  // ← новое
            args.degradationSpec = argv[++i];
            
        } else if (arg == "--csv" && i+1 < argc) {
            args.csvOutput = argv[++i];
        }
    }
    return args;
}

// === Справка по использованию ===
void printHelp() {
    std::cout << R"(
Scientific Simulator CLI (Workload-based Model with Stretching Method)

Usage: ./simulator [options]

Distribution format: type:param1,param2
  Types: exp, norm, gamma, lognorm, det, unif

Degradation function format: type:param
  Types: 
    hyp:R0     — гиперболическая: f(R) = 1/(1 + R/R0)
    exp:alpha  — экспоненциальная: f(R) = exp(-alpha*R)
    lin:Rmax   — линейная с обрезкой: f(R) = max(0.1, 1 - R/Rmax)
    thr:Rt,min — пороговая: f(R) = 1 if R<=Rt else min + (1-min)*Rt/R

Options:
  --users N           Number of users (closed system)
  --time T            Simulation time in seconds
  --seed S            Random seed for reproducibility
  --workload SPEC     Workload distribution per task (volume, not time!)
  --service-time SPEC Service time distribution 
  --passive SPEC      Passive phase (idle time) distribution
  --base-rate MU0     Base service rate (work units per second)
  --degradation FN    Degradation function specification
  --csv FILE          Save P(k) distribution to CSV (optional)
  --help, -h          Show this help

Examples:
  # Базовый запуск с экспоненциальным объёмом работы
  ./simulator --workload "exp:1.0" --passive "exp:0.5" --base-rate 1.0

  # Гамма-распределённый объём работы + гиперболическая деградация
  ./simulator --users 20 --time 1e5 --seed 123 \
              --workload "gamma:2,0.5" \
              --passive "exp:0.333" \
              --base-rate 1.0 \
              --degradation "hyp:10.0" \
              --csv results.csv

  # Детерминированный объём работы + экспоненциальная деградация
  ./simulator --workload "det:2.0" --degradation "exp:0.05" --base-rate 2.0
)";
}

// === Фабрика функций деградации ===
std::function<double(double)> parseDegradationFn(const std::string& spec) {
    auto parts = Cli::split(spec, ':');
    if (parts.empty()) {
        throw std::invalid_argument("Empty degradation spec");
    }
    
    const std::string& type = parts[0];
    
    // Гиперболическая: f(R) = 1 / (1 + R/R0)
    if (type == "hyp" || type == "hyperbolic") {
        if (parts.size() < 2) 
            throw std::invalid_argument("hyperbolic requires 1 param: R0");
        double R0 = std::stod(parts[1]);
        if (R0 <= 0) throw std::invalid_argument("R0 must be positive");
        return [R0](double R) -> double {
            return 1.0 / (1.0 + R / R0);
        };
    }
    
    // Экспоненциальная: f(R) = exp(-alpha * R)
    if (type == "exp" || type == "exponential") {
        if (parts.size() < 2) 
            throw std::invalid_argument("exponential requires 1 param: alpha");
        double alpha = std::stod(parts[1]);
        if (alpha < 0) throw std::invalid_argument("alpha must be non-negative");
        return [alpha](double R) -> double {
            return std::exp(-alpha * R);
        };
    }
    
    // Линейная с обрезкой: f(R) = max(minFactor, 1 - R/Rmax)
    if (type == "lin" || type == "linear") {
        if (parts.size() < 2) 
            throw std::invalid_argument("linear requires 1 param: Rmax");
        double Rmax = std::stod(parts[1]);
        if (Rmax <= 0) throw std::invalid_argument("Rmax must be positive");
        const double minFactor = 0.1;  // минимальный множитель скорости
        return [Rmax, minFactor](double R) -> double {
            return std::max(minFactor, 1.0 - R / Rmax);
        };
    }
    
    // Пороговая: f(R) = 1 if R<=Rt else min + (1-min)*Rt/R
    if (type == "thr" || type == "threshold") {
        if (parts.size() < 3) 
            throw std::invalid_argument("threshold requires 2 params: Rt, minFactor");
        double Rt = std::stod(parts[1]);
        double minFactor = std::stod(parts[2]);
        if (Rt <= 0 || minFactor < 0 || minFactor > 1) 
            throw std::invalid_argument("Invalid threshold params");
        return [Rt, minFactor](double R) -> double {
            return (R <= Rt) ? 1.0 : minFactor + (1.0 - minFactor) * Rt / R;
        };
    }
    
    throw std::invalid_argument("Unknown degradation type: " + type);
}

// === Точка входа ===
int main(int argc, char* argv[]) {
    auto args = parseArgs(argc, argv);
    
    if (args.help) {
        printHelp();
        return 0;
    }

    // Валидация входных параметров
    if (args.users <= 0) {
        std::cerr << "Error: --users must be positive\n";
        return 1;
    }
    if (args.simTime <= 0) {
        std::cerr << "Error: --time must be positive\n";
        return 1;
    }
    if (args.baseRate <= 0) {
        std::cerr << "Error: --base-rate must be positive\n";
        return 1;
    }

    // Инициализация ГСЧ
    RandomGenerator::instance().setSeed(args.seed);
    
    // Информационный вывод
    std::cout << "=== Scientific Simulator (Stretching Method) ===\n"
              << "Users:        " << args.users << "\n"
              << "Time:         " << args.simTime << " sec\n"
              << "Seed:         " << args.seed << "\n"
              << "Base rate:    " << args.baseRate << " work/sec\n"
              << "Workload:     " << args.workloadDist << "\n"
              << "Service time: " << args.serviceTimeDist << "\n"
              << "Passive:      " << args.passiveDist << "\n"
              << "Degradation:  " << args.degradationSpec << "\n\n";

    try {
        // === Парсинг распределений ===
        auto workloadCfg = Cli::parseDist(args.workloadDist);
        auto passiveCfg = Cli::parseDist(args.passiveDist);
        auto serviceCfg = Cli::parseDist(args.serviceTimeDist);
        
        auto workloadDist = Cli::createDist(workloadCfg);   // распределение ОБЪЁМА работы W_i
        auto serviceTimeDist = Cli::createDist(serviceCfg);
        auto passiveDist = Cli::createDist(passiveCfg);     // распределение времени простоя
        
        // === Создание функции деградации ===
        auto degradationFn = parseDegradationFn(args.degradationSpec);
        
        // === Создание и запуск симулятора ===
        // Конструктор: (maxUsers, baseRate, workloadDist, passiveDist, degradationFn)
        Simulator sim(
            args.users,
            args.baseRate,
            std::move(workloadDist),
            std::move(passiveDist),
            std::move(serviceTimeDist), 
            degradationFn
        );

        CsvStatisticsCollector csvCollector("simulation_data.csv");

        sim.attachListener(&csvCollector);
        
        sim.runUntil(args.simTime);
        
        // === Вывод результатов ===
        sim.getStats().printSummary(args.users);
        
        // === Сохранение распределения P(k) в CSV ===
        if (!args.csvOutput.empty()) {
            saveDistributionToCSV(
                sim.getStats().getProbabilityDistribution(), 
                args.csvOutput
            );
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        printHelp();
        return 1;
    }
    
    return 0;
}