#pragma once

#include "Observer.h"
#include <vector>
#include <algorithm>

using ObserverContainer = std::vector<Observer*>;

class Communicator {
public:
    ~Communicator() { m_observers.clear(); }

    bool addObserver(Observer* l_observer) {
        if (hasObserver(l_observer)) return false;
        m_observers.emplace_back(l_observer);
        return true;
    }

    bool removeObserver(Observer* l_observer) {
        auto itr = std::find_if(m_observers.begin(), m_observers.end(), [&l_observer](Observer* o) {
            return o == l_observer;
        });
        
        if (itr == m_observers.end()) return false;
        m_observers.erase(itr);
        return true;
    }

    bool hasObserver(const Observer* l_observer) {
        return (std::find_if(m_observers.begin(), m_observers.end(), [&l_observer](Observer* o) {
            return o == l_observer;
        }) != m_observers.end());
    }

    void broadcast(const Message& l_message) {
        for (auto &itr : m_observers) {
            itr->notify(l_message);
        }
    }
private:
    ObserverContainer m_observers;
};