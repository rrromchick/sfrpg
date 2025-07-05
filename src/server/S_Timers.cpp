#include "S_Timers.h"
#include "System_Manager.h"

S_Timers::S_Timers(SystemManager* l_sysMgr) : S_Base(System::Timers, l_sysMgr) {
    Bitmask req;

    req.turnOnBit((unsigned int)Component::State);
    req.turnOnBit((unsigned int)Component::Health);
    m_requiredComponents.emplace_back(req);
    req.clearBit((unsigned int)Component::Health);
    req.turnOnBit((unsigned int)Component::Attacker);

    req.clear();
}

S_Timers::~S_Timers() {}

void S_Timers::update(float l_dT) {
    EntityManager* entityManager = m_systemManager->getEntityManager();

    for (auto& itr : m_entities) {
        C_State* state = entityManager->getComponent<C_State>(itr, Component::State);

        if (state->getState() == EntityState::Hurt || state->getState() == EntityState::Dying) {
            C_Health* health = entityManager->getComponent<C_Health>(itr, Component::Health);
            health->addToTimer(sf::milliseconds(l_dT));
            
            if ((state->getState() == EntityState::Hurt && health->getTimer().asMilliseconds() < health->getHurtDuration()) ||
                (state->getState() == EntityState::Dying && health->getTimer().asMilliseconds() < health->getDeathDuration()))
            { continue; }

            health->reset();

            if (state->getState() == EntityState::Dying) {
                health->resetHealth();

                Message msg((MessageType)EntityMessage::Respawn);
                msg.m_receiver = itr;
                m_systemManager->getMessageHandler()->dispatch(msg);
            }
        } else if (state->getState() == EntityState::Attacking) {
            C_Attacker* attacker = entityManager->getComponent<C_Attacker>(itr, Component::Attacker);
            attacker->addToTimer(sf::milliseconds(l_dT));

            if (attacker->getTimer().asMilliseconds() < attacker->getAttackDuration()) continue;
            attacker->reset();
            attacker->setAttacked(false);
        } else { continue; }

        m_systemManager->getSystem<S_State>(System::State)->changeState(itr, EntityState::Idle, true);
    }
}

void S_Timers::handleEvent(const EntityId& l_entity, const EntityEvent& l_event) {}
void S_Timers::notify(const Message& l_message) {}