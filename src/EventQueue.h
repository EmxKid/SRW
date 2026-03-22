#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "Event.h"
#include <set>
#include <vector>
#include <functional>
#include <cstdint>
#include <iostream>

class EventQueue {
private:
    std::set<Event> events_;
    uint64_t nextSequenceId_ = 0;  // для уникальности sequenceId
    
public:
    // 🔥 ОБНОВЛЕНО: добавлен параметр eventVersion
    void push(double time, EventType type, int userId, uint64_t eventVersion, std::function<void()> handler);
    
    Event pop();
    const Event& peek() const;
    bool empty() const;
    size_t size() const;
    
    void debugPrint(size_t n = 5) const;
};

#endif // EVENT_QUEUE_H