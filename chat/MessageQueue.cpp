#include "MessageQueue.hpp"
#include <iostream>

void MessageQueue::push(const std::string& l_message) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.push(l_message);
    m_condVar.notify_one();
}

const std::string& MessageQueue::pop(bool l_block) {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (l_block) {
        m_condVar.wait(lock, [this]() { return !m_queue.empty() || m_stopping; });
    }

    if (m_queue.empty() || m_stopping) {
        return "";
    }

    std::string msg = m_queue.front();
    m_queue.pop();
    return msg;
}

bool MessageQueue::isEmpty() { 
    std::unique_lock<std::mutex> lock(m_mutex);
    return !m_queue.empty();
}

void MessageQueue::stop() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_stopping = true;
    m_condVar.notify_all();
}