#pragma once

#include "S_Base.h"
#include "C_Movable.h"
#include "Directions.h"

class S_Control : public S_Base {
public:
    S_Control(SystemManager* l_sysMgr);
    ~S_Control();

    void update(float l_dT);
    void handleEvent(const EntityId& l_entity, const EntityEvent& l_event);
    void notify(const Message& l_message);
    void moveEntity(const EntityId& l_entity, const Direction& l_direction);
private:

};