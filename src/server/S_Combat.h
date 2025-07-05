#pragma once

#include "S_Base.h"
#include "C_Position.h"
#include "C_Movable.h"
#include "S_State.h"
#include "C_Health.h"
#include "C_Attacker.h"

class S_Combat : public S_Base {
public:
    S_Combat(SystemManager* l_sysMgr);
    ~S_Combat();

    void update(float l_dT);
    void handleEvent(const EntityId& l_entity, const EntityEvent& l_event);
    void notify(const Message& l_message);
private:

};