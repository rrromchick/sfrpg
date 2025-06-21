#pragma once

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

class MessageQueue {
public:
    void push(const std::string& l_message);
    
    const std::string& pop(bool l_block = true);

    bool isEmpty();

    void stop();
private:
    std::queue<std::string> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    bool m_stopping = false;
};