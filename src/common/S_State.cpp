#include "S_State.h"
#include "System_Manager.h"

S_State::S_State(SystemManager* l_sysMgr) : S_Base(System::State, l_sysMgr) {
    Bitmask req;
    
    req.turnOnBit((unsigned int)Component::State);
    m_requiredComponents.emplace_back(req);

    m_systemManager->getMessageHandler()->subscribe(EntityMessage::Move, this);
    m_systemManager->getMessageHandler()->subscribe(EntityMessage::Switch_State, this);
    m_systemManager->getMessageHandler()->subscribe(EntityMessage::Attack, this);
    m_systemManager->getMessageHandler()->subscribe(EntityMessage::Hurt, this);
}

S_State::~S_State() {}

void S_State::update(float l_dT) {
    EntityManager* entityManager = m_systemManager->getEntityManager();
    for (auto &entity : m_entities) {
        C_State* state = entityManager->getComponent<C_State>(entity, Component::State);
        if (state->getState() != EntityState::Walking) continue;

        Message msg((MessageType)EntityMessage::Is_Moving);
        msg.m_receiver = entity;
        m_systemManager->getMessageHandler()->dispatch(msg);
    }
}

void S_State::handleEvent(const EntityId& l_entity, const EntityEvent& l_event) {
    switch (l_event) {
        case EntityEvent::Became_Idle: {
            changeState(l_entity, EntityState::Idle, false);
            break;
        }
    }
}

void S_State::notify(const Message& l_message) {
    if (!hasEntity(l_message.m_receiver)) return;
    
    EntityManager* entityManager = m_systemManager->getEntityManager();
    EntityMessage m = (EntityMessage)l_message.m_type;

    switch (m) {
        case EntityMessage::Move: {
            C_State* state = entityManager->getComponent<C_State>(l_message.m_receiver, Component::State);
            if (state->getState() == EntityState::Attacking || state->getState() == EntityState::Dying) return;

            EntityEvent e;
            if (l_message.m_int == (int)Direction::Left) {
                e = EntityEvent::Moving_Left;
            } else if (l_message.m_int == (int)Direction::Right) {
                e = EntityEvent::Moving_Right;
            } else if (l_message.m_int == (int)Direction::Up) {
                e = EntityEvent::Moving_Up;
            } else if (l_message.m_int == (int)Direction::Down) {
                e = EntityEvent::Moving_Down;
            }

            m_systemManager->addEvent(l_message.m_receiver, (EventID)e);
            changeState(l_message.m_receiver, EntityState::Walking, false);
            break;
        }
        case EntityMessage::Attack: {
            C_State* state = entityManager->getComponent<C_State>(l_message.m_receiver, Component::State);

            if (state->getState() != EntityState::Dying && state->getState() != EntityState::Attacking) {
                m_systemManager->addEvent(l_message.m_receiver, (EventID)EntityEvent::Began_Attacking);
                changeState(l_message.m_receiver, EntityState::Attacking, false);
            }

            break;
        }
        case EntityMessage::Hurt: {
            C_State* state = entityManager->getComponent<C_State>(l_message.m_receiver, Component::State);

            if (state->getState() != EntityState::Dying) {
                changeState(l_message.m_receiver, EntityState::Hurt, false);
            }

            break;
        }
        case EntityMessage::Switch_State: {
            changeState(l_message.m_receiver, (EntityState)l_message.m_int, false);
            break;
        }
    }
}

void S_State::changeState(const EntityId& l_entity, const EntityState& l_state, bool l_force) {
    EntityManager* entityManager = m_systemManager->getEntityManager();
    C_State* state = entityManager->getComponent<C_State>(l_entity, Component::State);

    if (l_state == state->getState()) return;
    if (!l_force && state->getState() == EntityState::Dying) return;

    state->setState(l_state);

    Message msg((MessageType)EntityMessage::Switch_State);
    msg.m_receiver = l_entity;
    msg.m_int = (int)l_state;
    m_systemManager->getMessageHandler()->dispatch(msg);
}

EntityState S_State::getState(const EntityId& l_entity) {
    EntityManager* entityManager = m_systemManager->getEntityManager();
    C_State* state = entityManager->getComponent<C_State>(l_entity, Component::State);
    return state->getState();
}