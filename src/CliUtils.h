// === CliUtils.h ===
#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "Distribution.h"

namespace Cli {

// Простой сплиттер строки по разделителю
inline std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
        if (!token.empty()) tokens.emplace_back(token);
    }
    return tokens;
}

// Парсинг распределения из строки "type:param1,param2"
struct DistConfig {
    std::string type;
    std::vector<double> params;
};

inline DistConfig parseDist(const std::string& spec) {
    auto parts = split(spec, ':');
    if (parts.empty()) throw std::invalid_argument("Empty distribution spec");
    
    DistConfig cfg;
    cfg.type = parts[0];
    
    if (parts.size() > 1) {
        auto params = split(parts[1], ',');
        cfg.params.reserve(params.size());
        for (const auto& p : params) {
            cfg.params.emplace_back(std::stod(p));
        }
    }
    return cfg;
}

// Фабрика распределений из конфига (делегирование на ваш DistributionFactory)
inline auto createDist(const DistConfig& cfg) {
    if (cfg.type == "exp" || cfg.type == "exponential") {
        if (cfg.params.size() != 1) 
            throw std::invalid_argument("exponential requires 1 param: rate");
        return DistributionFactory::exponential(cfg.params[0]);
    }
    if (cfg.type == "norm" || cfg.type == "normal") {
        if (cfg.params.size() != 2) 
            throw std::invalid_argument("normal requires 2 params: mean,std");
        return DistributionFactory::normal(cfg.params[0], cfg.params[1]);
    }
    if (cfg.type == "gamma") {
        if (cfg.params.size() != 2) 
            throw std::invalid_argument("gamma requires 2 params: shape,scale");
        return DistributionFactory::gamma(cfg.params[0], cfg.params[1]);
    }
    if (cfg.type == "lognorm" || cfg.type == "lognormal") {
        if (cfg.params.size() != 2) 
            throw std::invalid_argument("lognormal requires 2 params: mu,sigma");
        return DistributionFactory::lognormal(cfg.params[0], cfg.params[1]);
    }
    if (cfg.type == "det" || cfg.type == "deterministic") {
        if (cfg.params.size() != 1) 
            throw std::invalid_argument("deterministic requires 1 param: value");
        return DistributionFactory::deterministic(cfg.params[0]);
    }
    if (cfg.type == "unif" || cfg.type == "uniform") {
        if (cfg.params.size() != 2) 
            throw std::invalid_argument("uniform requires 2 params: min,max");
        return DistributionFactory::uniform(cfg.params[0], cfg.params[1]);
    }
    throw std::invalid_argument("Unknown distribution type: " + cfg.type);
}

} // namespace Cli