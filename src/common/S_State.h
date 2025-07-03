#pragma once

#include "S_Base.h"
#include "C_State.h"
#include "Directions.h"

class S_State : public S_Base {
public:
    S_State(SystemManager* l_sysMgr);
    ~S_State();

    void update(float l_dT);
    void handleEvent(const EntityId& l_entity, const EntityEvent& l_event);
    void notify(const Message& l_message);

    void changeState(const EntityId& l_entity, const EntityState& l_state, bool l_force);
    EntityState getState(const EntityId& l_entity);
private:

};