#pragma once

#include "S_Base.h"
#include "S_State.h"
#include "C_Health.h"
#include "C_Attacker.h"

class S_Timers : public S_Base {
public:
    S_Timers(SystemManager* l_sysMgr);
    ~S_Timers();

    void update(float l_dT);
    void handleEvent(const EntityId& l_entity, const EntityEvent& l_event);
    void notify(const Message& l_message);
private:

};