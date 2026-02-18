#include "EventQueue.h"
#include <iostream>
#include <stdexcept>

void EventQueue::push(const Event& event) {
    queue_.push(event);
}

Event EventQueue::pop() {
    if (empty()) throw std::runtime_error("Pop from empty event queue");
    Event event = queue_.top();
    queue_.pop();
    return event;
}

const Event& EventQueue::peek() const {
    if (empty()) throw std::runtime_error("Peek from empty event queue");
    return queue_.top();
}

bool EventQueue::empty() const {
    return queue_.empty();
}

size_t EventQueue::size() const {
    return queue_.size();
}

void EventQueue::debugPrint(size_t n) const {
    auto copy = queue_;
    std::cout << "=== Event Queue (next " << n << ") ===\n";
    size_t i = 0;
    while (!copy.empty() && i < n) {
        const auto& e = copy.top();
        std::cout << "[" << i << "] t=" << e.time 
                  << " type=" << static_cast<int>(e.type) 
                  << " userId=" << e.userId << "\n";
        copy.pop();
        ++i;
    }
    std::cout << "===========================\n";
}