#pragma once

#include <queue>

using EventID = int;

class EventQueue {
public:
    void addEvent(const EventID& l_event) { m_queue.push(l_event); }

    bool processEvents(EventID& l_event) {
        if (m_queue.empty()) return false;
        l_event = m_queue.front();
        m_queue.pop();
        return true;
    }

    void clear() { while (!m_queue.empty()) { m_queue.pop(); } }
private:
    std::queue<EventID> m_queue;
};