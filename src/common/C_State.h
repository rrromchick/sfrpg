#pragma once

#include "C_Base.h"

enum class EntityState { Idle = 0, Walking, Hurt, Attacking, Dying };

class C_State : public C_Base {
public:
    C_State() : C_Base(Component::State)
    {}

    void readIn(std::stringstream& l_stream) {
        unsigned int state = 0;

        l_stream >> state;
        m_state = (EntityState)state;
    }

    EntityState getState() { return m_state; }
    void setState(const EntityState& l_state) { m_state = l_state; }
private:
    EntityState m_state;
};