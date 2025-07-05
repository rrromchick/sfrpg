#pragma once

#include <SFML/Graphics/Rect.hpp>
#include "S_Base.h"
#include "C_Collidable.h"
#include "C_Position.h"

struct TileInfo;
class Map;

struct CollisionElement {
    CollisionElement() : m_area(0), m_tile(nullptr) {}

    float m_area;
    TileInfo* m_tile;
    sf::FloatRect m_tileBounds;
};

using Collisions = std::vector<CollisionElement>;

class S_Collision : public S_Base {
public:
    S_Collision(SystemManager* l_sysMgr);
    ~S_Collision();

    void setMap(Map* l_map);
    
    void update(float l_dT);
    void handleEvent(const EntityId& l_entity, const EntityEvent& l_event);
    void notify(const Message& l_message);
private:    
    void entityCollisions();
    void mapCollisions(const EntityId& l_entity, C_Position* l_pos, C_Collidable* l_col);
    void checkOutOfBounds(C_Position* l_pos, C_Collidable* l_col);

    Map* m_gameMap;
};