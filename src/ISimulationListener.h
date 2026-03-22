#pragma once
#include <vector>

struct SimulationSnapshot {
    double time;
    int activeUsers;
    double totalWorkload;
    double effectiveRate;
    double degradationFactor;
    // Можно добавить другие метрики по требованию
};

class ISimulationListener {
public:
    virtual ~ISimulationListener() = default;
    virtual void onSnapshot(const SimulationSnapshot& snapshot) = 0;
    virtual void onSimulationEnd() = 0; // Для финализации (закрытия файла)
};