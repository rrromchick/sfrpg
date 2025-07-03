#include "System_Manager.h"
#include "Entity_Manager.h"

SystemManager::SystemManager() { m_entityManager = nullptr; }
SystemManager::~SystemManager() { purgeSystems(); }

EntityManager* SystemManager::getEntityManager() { return m_entityManager; }
MessageHandler* SystemManager::getMessageHandler() { return &m_messages; }

void SystemManager::setEntityManager(EntityManager* l_entityMgr) { 
    if (!m_entityManager) m_entityManager = l_entityMgr; 
}

void SystemManager::update(float l_dT) {
    for (auto &s_itr : m_systems) {
        s_itr.second->update(l_dT);
    }
    handleEvents();
}

void SystemManager::handleEvents() {
    for (auto &event : m_events) {
        EventID id = 0;
        while (event.second.processEvents(id)) {
            for (auto &s_itr : m_systems) {
                if (s_itr.second->hasEntity(event.first)) {
                    s_itr.second->handleEvent(event.first, (EntityEvent)id);
                }
            }
        }
    }
}

void SystemManager::entityModified(const EntityId& l_entity, const Bitmask& l_bits) {
    for (auto &s_itr : m_systems) {
        if (!s_itr.second->hasEntity(l_entity)) {
            if (s_itr.second->fitsRequirements(l_bits)) {
                s_itr.second->addEntity(l_entity);
            }
        } else {
            if (!s_itr.second->fitsRequirements(l_bits)) {
                s_itr.second->removeEntity(l_entity);
            }
        }
    }
}

void SystemManager::removeEntity(const EntityId& l_entity) {
    for (auto &s_itr : m_systems) {
        s_itr.second->removeEntity(l_entity);
    }
}

void SystemManager::addEvent(const EntityId& l_entity, const EventID& l_event) {
    m_events[l_entity].addEvent(l_event);
}

void SystemManager::purgeEntities() {
    for (auto &s_itr : m_systems) {
        s_itr.second->purge();
    }
}

void SystemManager::purgeSystems() {
    for (auto &s_itr : m_systems) {
        delete s_itr.second;
    }
    m_systems.clear();
}