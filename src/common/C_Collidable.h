#pragma once

#include "C_Base.h"
#include <SFML/Graphics/Rect.hpp>

enum class Origin { Top_Left, Abs_Centre, Mid_Bottom };

class C_Collidable : public C_Base {
public:
    C_Collidable() : C_Base(Component::Collidable), m_origin(Origin::Mid_Bottom), 
        m_collidingOnX(false), m_collidingOnY(false)
    {}

    void readIn(std::stringstream& l_stream) {
        unsigned int origin = 0;
        
        l_stream >> m_aabb.width >> m_aabb.height >> m_offset.x 
            >> m_offset.y >> origin;
        
        m_origin = (Origin)origin;
    }

    const sf::FloatRect& getCollidable() { return m_aabb; }
    void setCollidable(const sf::FloatRect& l_rect) { m_aabb = l_rect; }
    
    bool isCollidingOnX() const { return m_collidingOnX; }
    bool isCollidingOnY() const { return m_collidingOnY; }

    void collideOnX() { m_collidingOnX = true; }
    void collideOnY() { m_collidingOnY = true; }

    void resetCollisionFlags() {
        m_collidingOnX = false;
        m_collidingOnY = false;
    }

    void setSize(const sf::Vector2f& l_vec) {
        m_aabb.width = l_vec.x;
        m_aabb.height = l_vec.y;
    }

    void setPosition(const sf::Vector2f& l_vec) {
        switch (m_origin) {
            case (Origin::Top_Left): {
                m_aabb.left = l_vec.x + m_offset.x;
                m_aabb.top = l_vec.y + m_offset.y;
                break;
            }
            case (Origin::Abs_Centre): {
                m_aabb.left = l_vec.x - (m_aabb.width / 2) + m_offset.x;
                m_aabb.top = l_vec.y - (m_aabb.height / 2) + m_offset.y;
                break;
            }
            case (Origin::Mid_Bottom): {
                m_aabb.left = l_vec.x - (m_aabb.width / 2) + m_offset.x;
                m_aabb.top = l_vec.y - (m_aabb.height / 2) + m_offset.y;
                break;
            }
        }
    }
private:
    sf::FloatRect m_aabb;
    sf::Vector2f m_offset;
    Origin m_origin;

    bool m_collidingOnX;
    bool m_collidingOnY;
};