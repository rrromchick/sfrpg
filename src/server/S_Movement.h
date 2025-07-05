#pragma once

#include "S_Base.h"
#include "Directions.h"
#include "C_Movable.h"
#include "C_Position.h"

enum class Axis { x, y };

class Map;

class S_Movement : public S_Base {
public:
    S_Movement(SystemManager* l_systemMgr);
    ~S_Movement();

    void update(float l_dT);
    void handleEvent(const EntityId& l_entity, const EntityEvent& l_event);
    void notify(const Message& l_message);

    void stopEntity(const EntityId& l_entity, const Axis& l_axis);
    void setDirection(const EntityId& l_entity, const Direction& l_direction);
    void setMap(Map* l_map);
private:
    const sf::Vector2f& getTileFriction(unsigned int l_elevation, unsigned int l_x, unsigned int l_y);
    void movementStep(float l_dT, C_Movable* l_movable, C_Position* l_position);
    
    Map* m_gameMap;
};