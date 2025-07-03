#pragma once

#include "MessageHandler.h"
#include "Event_Queue.h"
#include "S_Base.h"
#include <unordered_map>

using SystemContainer = std::unordered_map<EntityId, S_Base*>;
using EntityEventContainer = std::unordered_map<EntityId, EventQueue>;

class EntityManager;

class SystemManager {
public:
    SystemManager();
    virtual ~SystemManager();

    EntityManager* getEntityManager();
    void setEntityManager(EntityManager* l_entityMgr);
    MessageHandler* getMessageHandler();

    template <class T>
    T* getSystem(const System& l_id) {
        auto itr = m_systems.find(l_id);
        return (itr != m_systems.end() ? dynamic_cast<T*>(itr->second) : nullptr);
    }

    template <class T>
    bool addSystem(const System& l_id) {
        if (m_systems.find(l_id) != m_systems.end()) return false;
        m_systems[l_id] = new T(this);
        return true;
    }

    void update(float l_dT);
    void handleEvents();

    void entityModified(const EntityId& l_entity, const Bitmask& l_bits);
    void removeEntity(const EntityId& l_entity);

    void addEvent(const EntityId& l_entity, const EventID& l_event);

    void purgeEntities();
    void purgeSystems();
protected:
    EntityManager* m_entityManager;

    SystemContainer m_systems;
    EntityEventContainer m_events;
    MessageHandler m_messages;
};