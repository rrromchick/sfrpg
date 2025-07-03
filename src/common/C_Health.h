#pragma once

#include "C_TimedComponentBase.h"

using Health = int;

class C_Health : public C_TimedComponentBase {
public:
    C_Health() : C_TimedComponentBase(Component::Health), m_hurtDuration(0), m_deathDuration(0)
    {}

    void readIn(std::stringstream& l_stream) {
        l_stream >> m_maxHealth >> m_hurtDuration >> m_deathDuration;
        m_health = m_maxHealth;
    }

    Health getHealth() { return m_health; }
    Health getMaxHealth() { return m_maxHealth; }
    void setHealth(const Health& l_health) { m_health = l_health; }
    void resetHealth() { m_health = m_maxHealth; }

    sf::Uint32 getHurtDuration() { return m_hurtDuration; }
    sf::Uint32 getDeathDuration() { return m_deathDuration; }
private:
    Health m_health;
    Health m_maxHealth;

    sf::Uint32 m_hurtDuration;
    sf::Uint32 m_deathDuration;
};