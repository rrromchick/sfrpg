#include "S_Combat.h"
#include "System_Manager.h"

S_Combat::S_Combat(SystemManager* l_sysMgr) : S_Base(System::Combat, l_sysMgr) {
    Bitmask req;

    req.turnOnBit((unsigned int)Component::Position);
    req.turnOnBit((unsigned int)Component::Movable);
    req.turnOnBit((unsigned int)Component::State);
    req.turnOnBit((unsigned int)Component::Health);
    m_requiredComponents.emplace_back(req);

    req.clearBit((unsigned int)Component::Health);
    req.turnOnBit((unsigned int)Component::Attacker);
    m_requiredComponents.emplace_back(req);

    m_systemManager->getMessageHandler()->subscribe(EntityMessage::Being_Attacked, this);
}

S_Combat::~S_Combat() {}

void S_Combat::update(float l_dT) {
    EntityManager* entityManager = m_systemManager->getEntityManager();
    
    for (auto& entity : m_entities) {
        C_Attacker* attacker = entityManager->getComponent<C_Attacker>(entity, Component::Attacker);
        if (!attacker) continue;

        sf::Vector2f offset = attacker->getOffset();
        sf::FloatRect aoa = attacker->getAreaOfAttack();
        sf::Vector2f position = entityManager->getComponent<C_Position>(entity, Component::Position)->getPosition();
        Direction dir = entityManager->getComponent<C_Movable>(entity, Component::Movable)->getDirection();

        if (dir == Direction::Left) offset.x -= aoa.width / 2;
        else if (dir == Direction::Right) offset.x += aoa.width / 2;
        else if (dir == Direction::Up) offset.y -= aoa.height / 2;
        else if (dir == Direction::Down) offset.y += aoa.height / 2;

        position -= sf::Vector2f(aoa.width / 2, aoa.height / 2);
        attacker->setAreaPosition(position + offset);
    }
}

void S_Combat::handleEvent(const EntityId& l_entity, const EntityEvent& l_event) {}

void S_Combat::notify(const Message& l_message) {
    if (!hasEntity(l_message.m_receiver) || !hasEntity(l_message.m_sender)) return;

    EntityManager* entityManager = m_systemManager->getEntityManager();
    EntityMessage m = (EntityMessage)l_message.m_type;

    switch (m) {
        case EntityMessage::Being_Attacked: {
            C_Health* victim = entityManager->getComponent<C_Health>(l_message.m_receiver, Component::Health);
            C_Attacker* attacker = entityManager->getComponent<C_Attacker>(l_message.m_sender, Component::Attacker);
            if (!victim || !attacker) return;

            S_State* stateSystem = m_systemManager->getSystem<S_State>(System::State);
            if (!stateSystem) return;
            if (attacker->hasAttacked()) return;

            victim->setHealth((victim->getHealth() > 1 ? victim->getHealth() - 1 : 0));
            attacker->setAttacked(true);
            if (!victim->getHealth()) {
                stateSystem->changeState(l_message.m_receiver, EntityState::Dying, true);
            } else {
                Message msg((MessageType)EntityMessage::Hurt);
                msg.m_receiver = l_message.m_receiver;
                m_systemManager->getMessageHandler()->dispatch(msg);
            }

            C_Movable* mov = entityManager->getComponent<C_Movable>(l_message.m_sender, Component::Movable);
            if (!mov) return;

            float knockback = attacker->getKnockback();
            if (mov->getDirection() == Direction::Left || mov->getDirection() == Direction::Up) knockback = -knockback;

            sf::Vector2f knockbackVelocity;
            if (mov->getDirection() == Direction::Left || mov->getDirection() == Direction::Right) knockbackVelocity.x = knockback;
            else knockbackVelocity.y = knockback;

            entityManager->getComponent<C_Movable>(l_message.m_receiver, Component::Movable)->setVelocity(knockbackVelocity);
            break;
        }
    }
}