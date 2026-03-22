#include "EventQueue.h"
#include <stdexcept>

void EventQueue::push(double time, EventType type, int userId, uint64_t eventVersion, std::function<void()> handler) {
    Event event(time, type, userId, nextSequenceId_++, eventVersion, std::move(handler));
    events_.insert(event);
}

Event EventQueue::pop() {
    if (empty()) throw std::runtime_error("Pop from empty event queue");
    auto it = events_.begin();
    Event event = *it;
    events_.erase(it);
    return event;
}

const Event& EventQueue::peek() const {
    if (empty()) throw std::runtime_error("Peek from empty event queue");
    return *events_.begin();
}

bool EventQueue::empty() const {
    return events_.empty();
}

size_t EventQueue::size() const {
    return events_.size();
}

void EventQueue::debugPrint(size_t n) const {
    std::cout << "=== Event Queue (next " << n << ") ===\n";
    size_t i = 0;
    for (const auto& e : events_) {
        if (i++ >= n) break;
        std::cout << "[" << i-1 << "] t=" << e.time 
                  << " type=" << static_cast<int>(e.type) 
                  << " userId=" << e.userId 
                  << " ver=" << e.eventVersion
                  << " seq=" << e.sequenceId << "\n";
    }
    std::cout << "===========================\n";
}