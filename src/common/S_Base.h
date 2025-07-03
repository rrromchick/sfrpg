#pragma once

#include "Entity_Manager.h"
#include "Bitmask.h"
#include "EntityEvents.h"
#include "Observer.h"
#include "ECS_Types.h"

using EntityList = std::vector<EntityId>;
using Requirements = std::vector<Bitmask>;

class SystemManager;

class S_Base : public Observer {
public:
    S_Base(const System& l_id, SystemManager* l_systemMgr);
    virtual ~S_Base() {}

    bool addEntity(const EntityId& l_entity);
    bool hasEntity(const EntityId& l_entity);
    bool removeEntity(const EntityId& l_entity);

    System getId();

    bool fitsRequirements(const Bitmask& l_bits);
    void purge();

    virtual void update(float l_dT) = 0;
    virtual void handleEvent(const EntityId& l_entity, const EntityEvent& l_event) = 0;
protected:
    System m_id;

    EntityList m_entities;
    Requirements m_requiredComponents;
    SystemManager* m_systemManager;
};