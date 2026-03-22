// CsvStatisticsCollector.h
#pragma once
#include "ISimulationListener.h"
#include <fstream>
#include <iomanip>
#include <memory>

class CsvStatisticsCollector : public ISimulationListener {
    std::unique_ptr<std::ofstream> m_file;
    bool m_headerWritten = false;

public:
    explicit CsvStatisticsCollector(const std::string& filename) {
        m_file = std::make_unique<std::ofstream>(filename);
        if (!m_file->is_open()) {
            throw std::runtime_error("Cannot open CSV file: " + filename);
        }
    }

    void onSnapshot(const SimulationSnapshot& snapshot) override {
        if (!m_headerWritten) {
            *m_file << "time,active_users,total_workload,effective_rate,"
                    << "degradation_factor\n";
            m_headerWritten = true;
        }

        *m_file << std::fixed << std::setprecision(4)
                << snapshot.time << ","
                << snapshot.activeUsers << ","
                << snapshot.totalWorkload << ","
                << snapshot.effectiveRate << ","
                << snapshot.degradationFactor << "\n";
    }

    void onSimulationEnd() override {
        if (m_file && m_file->is_open()) {
            m_file->flush();
            m_file->close();
        }
    }
};