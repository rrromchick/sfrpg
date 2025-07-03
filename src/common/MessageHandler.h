#pragma once

#include "EntityMessages.h"
#include "Communicator.h"
#include <unordered_map>

using Subscribtions = std::unordered_map<EntityMessage, Communicator>;

class MessageHandler {
public:
    bool subscribe(const EntityMessage& l_message, Observer* l_observer) {
        return m_communicators[l_message].addObserver(l_observer);
    }

    bool unsubscribe(const EntityMessage& l_message, Observer* l_observer) {
        return m_communicators[l_message].removeObserver(l_observer);
    }

    void dispatch(const Message& l_message) {
        auto itr = m_communicators.find((EntityMessage)l_message.m_type);
        if (itr == m_communicators.end()) return;
        itr->second.broadcast(l_message);
    }
private:
    Subscribtions m_communicators;
};