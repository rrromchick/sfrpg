#pragma once

#include "C_Base.h"
#include "Utilities.h"
#include "Bitmask.h"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <functional>
#include <iostream>

using EntityId = int;

using ComponentContainer = std::vector<C_Base*>;

struct EntityData {
    Bitmask m_bitmask;
    std::string m_type;
    ComponentContainer m_components;
};

using EntityContainer = std::unordered_map<EntityId, EntityData>;
using ComponentFactory = std::unordered_map<Component, std::function<C_Base*(void)>>;

class SystemManager;

class EntityManager {
public:
    EntityManager(SystemManager* l_systemMgr);
    virtual ~EntityManager();

    void setSystemManager(SystemManager* l_systemMgr);

    int addEntity(const Bitmask& l_bitmask, int l_id = -1);
    virtual int addEntity(const std::string& l_entityFile, int l_id = -1);
    bool removeEntity(const EntityId& l_entity);

    template <class T>
    T* getComponent(const EntityId& l_entity, const Component& l_component) {
        auto itr = m_entities.find(l_entity);
        if (itr == m_entities.end()) return nullptr;

        if (!itr->second.m_bitmask.getBit((unsigned int)l_component)) return nullptr;

        auto& container = itr->second.m_components;
        auto component = std::find_if(container.begin(), container.end(), [&l_component](C_Base* c) {
            return c->getType() == l_component;
        });

        return (component != container.end() ? dynamic_cast<T*>(*component) : nullptr);
    }

    bool hasEntity(const EntityId& l_entity);

    bool addComponent(const EntityId& l_entity, const Component& l_component);
    bool hasComponent(const EntityId& l_entity, const Component& l_component);
    bool removeComponent(const EntityId& l_entity, const Component& l_component);

    void purge();
    
    unsigned int getEntityCount() const;
protected:
    template <class T>
    void addComponentType(const Component& l_type) {
        m_compFactory[l_type] = []() -> C_Base* { return new T(); };
    }

    unsigned int m_idCounter;
    
    EntityContainer m_entities;
    ComponentFactory m_compFactory;
    SystemManager* m_systemManager;
};