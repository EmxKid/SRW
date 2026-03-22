// === CsvUtils.h ===
#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

void saveDistributionToCSV(const std::vector<double>& pk, const std::string& filename) {
    std::ofstream csv(filename);
    if (csv.is_open()) {
        csv << "k,P(k)\n";
        for (size_t k = 0; k < pk.size(); ++k) {
            csv << k << "," << pk[k] << "\n";
        }
        csv.close();
        std::cout << "  Distribution P(k) saved to " << filename << "\n";
    } else {
        std::cerr << "  Warning: Could not open file " << filename << " for writing\n";
    }
}