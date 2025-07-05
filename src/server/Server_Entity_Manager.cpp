#include "Server_Entity_Manager.h"
#include "System_Manager.h"

ServerEntityManager::ServerEntityManager(SystemManager* l_systemMgr) : EntityManager(l_systemMgr) {
    addComponentType<C_Attacker>(Component::Attacker);
    addComponentType<C_Position>(Component::Position);
    addComponentType<C_Movable>(Component::Movable);
    addComponentType<C_Collidable>(Component::Collidable);
    addComponentType<C_Controller>(Component::Controller);
    addComponentType<C_Client>(Component::Client);
    addComponentType<C_Name>(Component::Name);
    addComponentType<C_State>(Component::State);
    addComponentType<C_Health>(Component::Health);
}

ServerEntityManager::~ServerEntityManager() {}

void ServerEntityManager::dumpEntityInfo(sf::Packet& l_packet) {
    for (auto &itr : m_entities) {
        l_packet << (sf::Int32)itr.first;

        EntitySnapshot snapshot;
        snapshot.m_type = itr.second.m_type;

        const auto& mask = itr.second.m_bitmask;

        if (mask.getBit((unsigned int)Component::Position)) {
            C_Position* pos = getComponent<C_Position>(itr.first, Component::Position);
            snapshot.m_position = pos->getPosition();
            snapshot.m_elevation = pos->getElevation();
        }

        if (mask.getBit((unsigned int)Component::Movable)) {
            C_Movable* mov = getComponent<C_Movable>(itr.first, Component::Movable);
            snapshot.m_velocity = mov->getVelocity();
            snapshot.m_acceleration = mov->getAcceleration();
            snapshot.m_direction = (sf::Uint8)mov->getDirection();
        }

        if (mask.getBit((unsigned int)Component::State)) {
            C_State* state = getComponent<C_State>(itr.first, Component::State);
            snapshot.m_state = (sf::Uint8)state->getState();
        }

        if (mask.getBit((unsigned int)Component::Health)) {
            C_Health* health = getComponent<C_Health>(itr.first, Component::Health);
            snapshot.m_health = (sf::Uint8)health->getHealth();
        }

        if (mask.getBit((unsigned int)Component::Name)) {
            C_Name* name = getComponent<C_Name>(itr.first, Component::Name);
            snapshot.m_name = name->getName();
        }

        l_packet << snapshot;
    }
}