#include "Simulator.h"
#include "Distribution.h"
#include "RandomGenerator.h"
#include "CliUtils.h"  // ← новый хедер
#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_map>
#include <optional>

// Простой парсер аргументов командной строки
struct Args {
    int users;
    double simTime;
    int seed;
    std::string activeDist;
    std::string passiveDist;
    std::string csvOutput;
    bool help = false;
};

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
        } else if (arg == "--active" && i+1 < argc) {
            args.activeDist = argv[++i];
        } else if (arg == "--passive" && i+1 < argc) {
            args.passiveDist = argv[++i];
        } else if (arg == "--csv" && i+1 < argc) {
            args.csvOutput = argv[++i];
        }
    }
    return args;
}

void printHelp() {
    std::cout << R"(
Scientific Simulator CLI

Usage: ./simulator [options]

Distribution format: type:param1,param2
  Types: exp, norm, gamma, lognorm, det, unif

Options:
  --users N        Number of users
  --time T         Simulation time in seconds
  --seed S         Random seed for reproducibility
  --active SPEC    Active phase distribution 
  --passive SPEC   Passive phase distribution
  --csv FILE       Save P(k) distribution to CSV (optional)
  --help, -h       Show this help

Examples:
  ./simulator --active "exp:0.5" --passive "exp:0.3333"
  ./simulator --active "norm:2.0,0.5" --passive "det:3.0" --csv result.csv
  ./simulator --users 20 --time 5e5 --seed 123 --active "gamma:2,1"
)";
}

int main(int argc, char* argv[]) {
    auto args = parseArgs(argc, argv);
    
    if (args.help) {
        printHelp();
        return 0;
    }

    // Инициализация RNG
    RandomGenerator::instance().setSeed(args.seed);
    
    std::cout << "=== Scientific Simulator ===\n"
              << "Users: " << args.users 
              << ", Time: " << args.simTime 
              << ", Seed: " << args.seed << "\n"
              << "Active:  " << args.activeDist << "\n"
              << "Passive: " << args.passiveDist << "\n\n";

    try {
        // Парсинг и создание распределений
        auto activeCfg = Cli::parseDist(args.activeDist);
        auto passiveCfg = Cli::parseDist(args.passiveDist);
        
        auto activeDist = Cli::createDist(activeCfg);
        auto passiveDist = Cli::createDist(passiveCfg);
        
        // Запуск симуляции
        Simulator sim(args.users, std::move(activeDist), std::move(passiveDist));
        sim.runUntil(args.simTime);
        
        // Вывод результатов
        sim.getStats().printSummary(args.users);
        
        // Теоретический анализ (если нужна эта функция)
        // printTheoreticalAnalysis(sim.getStats());
        
        // Сохранение в CSV если указано
        if (!args.csvOutput.empty()) {
            saveDistributionToCSV(sim.getStats().getProbabilityDistribution(), args.csvOutput);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        printHelp();
        return 1;
    }
    
    return 0;
}