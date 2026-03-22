#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <memory>
#include <functional>
#include <cstdint>  // для uint64_t

enum class EventType {
    ACTIVATION,
    DEACTIVATION,
    MONITORING
};

struct Event {
    double time;
    EventType type;
    int userId;
    uint64_t sequenceId;  // ← НОВОЕ: уникальный ID для уникальности в std::set
    uint64_t eventVersion; // версия события для lazy deletion
    std::function<void()> handler;
    
    // Конструктор для удобства
    Event(double t, EventType ty, int uid, uint64_t seq, uint64_t ver, std::function<void()> h)
        : time(t), type(ty), userId(uid), sequenceId(seq), eventVersion(ver), handler(std::move(h)) {}
    
    // Компаратор для std::set: мин-куча по времени + приоритет по типу + уникальность
    bool operator<(const Event& other) const {
        if (time != other.time) return time < other.time;                    // 1. по времени
        if (static_cast<int>(type) != static_cast<int>(other.type))          // 2. по типу
            return static_cast<int>(type) < static_cast<int>(other.type);
        return sequenceId < other.sequenceId;                                // 3. по ID (гарантия уникальности)
    }
};

class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual Event createArrivalEvent(double currentTime, int userId) = 0;
    virtual Event createCompletionEvent(double currentTime, int userId, double workload) = 0;
};

#endif // EVENT_H