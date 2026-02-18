#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "Event.h"
#include <queue>
#include <vector>

class EventQueue {
private:
    // Мин-куча по времени события (с приоритетом по типу при равенстве)
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> queue_;
    
public:
    void push(const Event& event);
    Event pop();
    const Event& peek() const;
    bool empty() const;
    size_t size() const;
    
    // Отладочный вывод первых N событий
    void debugPrint(size_t n = 5) const;
};

#endif // EVENT_QUEUE_H