#include "S_Movement.h"
#include "System_Manager.h"
#include "Map.h"

S_Movement::S_Movement(SystemManager* l_sysMgr) : S_Base(System::Movement, l_sysMgr) {
    Bitmask req;
    
    req.turnOnBit((unsigned int)Component::Movable);
    req.turnOnBit((unsigned int)Component::Position);
    req.turnOnBit((unsigned int)Component::State);
    m_requiredComponents.emplace_back(req);

    m_systemManager->getMessageHandler()->subscribe(EntityMessage::Is_Moving, this);
}

S_Movement::~S_Movement() {}

void S_Movement::update(float l_dT) {
    EntityManager* entityManager = m_systemManager->getEntityManager();
    for (auto &entity : m_entities) {
        C_Movable* mov = entityManager->getComponent<C_Movable>(entity, Component::Movable);
        C_Position* pos = entityManager->getComponent<C_Position>(entity, Component::Position);
        movementStep(l_dT, mov, pos);
        pos->moveBy(mov->getVelocity());
    }
}

void S_Movement::handleEvent(const EntityId& l_entity, const EntityEvent& l_event) {
    if (!hasEntity(l_entity)) return;
    switch (l_event) {
        case EntityEvent::Colliding_X: stopEntity(l_entity, Axis::x); break;
        case EntityEvent::Colliding_Y: stopEntity(l_entity, Axis::y); break;
        case EntityEvent::Moving_Left: setDirection(l_entity, Direction::Left); break;
        case EntityEvent::Moving_Right: setDirection(l_entity, Direction::Right); break;
        case EntityEvent::Moving_Up: {
            C_Movable* mov = m_systemManager->getEntityManager()->getComponent<C_Movable>(l_entity, Component::Movable);
            if (mov->getVelocity().x == 0.f) setDirection(l_entity, Direction::Up);
            break;
        }
        case EntityEvent::Moving_Down: {
            C_Movable* mov = m_systemManager->getEntityManager()->getComponent<C_Movable>(l_entity, Component::Movable);
            if (mov->getVelocity().x == 0.f) setDirection(l_entity, Direction::Down);
            break;
        }
    }
}

void S_Movement::notify(const Message& l_message) {
    if (!hasEntity(l_message.m_receiver)) return;

    EntityManager* entityManager = m_systemManager->getEntityManager();
    EntityMessage m = (EntityMessage)l_message.m_type;

    switch (m) {
        case EntityMessage::Is_Moving: {
            C_Movable* mov = entityManager->getComponent<C_Movable>(l_message.m_receiver, Component::Movable);
            if (mov->getVelocity() != sf::Vector2f(0.f, 0.f)) return;
            m_systemManager->addEvent(l_message.m_receiver, (EventID)EntityEvent::Became_Idle);
            break;
        }
    }
}

const sf::Vector2f& S_Movement::getTileFriction(unsigned int l_layer, unsigned int l_x, unsigned int l_y) {
    Tile* tile = m_gameMap->getTile(l_x, l_y, l_layer);
    unsigned int elevation = l_layer;

    while (!tile && elevation > 0) {
        --elevation;
        tile = m_gameMap->getTile(l_x, l_y, elevation);
    }

    return (tile ? tile->m_properties->m_friction : sf::Vector2f(0.f, 0.f));
}

void S_Movement::movementStep(float l_dT, C_Movable* l_movable, C_Position* l_position) {
    sf::Vector2f f_coef(getTileFriction(l_position->getElevation(), floor(l_position->getPosition().x / (unsigned int)Sheet::Tile_Size),
        floor(l_position->getPosition().y / (unsigned int)Sheet::Tile_Size)));
    
    sf::Vector2f friction(l_movable->getSpeed().x * f_coef.x, l_movable->getSpeed().y * f_coef.y);

    l_movable->addVelocity(l_movable->getAcceleration() * l_dT);
    l_movable->setAcceleration(sf::Vector2f(0.f, 0.f));
    l_movable->applyFriction(friction * l_dT);

    float magnitude = sqrt(l_movable->getVelocity().x * l_movable->getVelocity().x +
        l_movable->getVelocity().y * l_movable->getVelocity().y);
    
    if (magnitude <= l_movable->getMaxVelocity()) return;
    float maxV = l_movable->getMaxVelocity();

    l_movable->setVelocity(sf::Vector2f((l_movable->getVelocity().x / magnitude) * maxV,
        (l_movable->getVelocity().y / magnitude) * maxV));
}

void S_Movement::stopEntity(const EntityId& l_entity, const Axis& l_axis) {
    C_Movable* mov = m_systemManager->getEntityManager()->getComponent<C_Movable>(l_entity, Component::Movable);

    if (l_axis == Axis::x) {
        mov->setVelocity(sf::Vector2f(0.f, mov->getVelocity().y));    
    } else if (l_axis == Axis::y) {
        mov->setVelocity(sf::Vector2f(mov->getVelocity().x, 0.f));
    }
}

void S_Movement::setMap(Map* l_map) { m_gameMap = l_map; }

void S_Movement::setDirection(const EntityId& l_entity, const Direction& l_direction) {
    C_Movable* mov = m_systemManager->getEntityManager()->getComponent<C_Movable>(l_entity, Component::Movable);
    mov->setDirection(l_direction);

    Message msg((MessageType)EntityMessage::Direction_Changed);
    msg.m_receiver = l_entity;
    msg.m_int = (int)l_direction;
    m_systemManager->getMessageHandler()->dispatch(msg);
}