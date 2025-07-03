#pragma once

#include "C_Base.h"
#include <SFML/System/Vector2.hpp>

class C_Position : public C_Base {
public:
    C_Position() : C_Base(Component::Position), m_elevation(0) 
    {}

    void readIn(std::stringstream& l_stream) {
        l_stream >> m_position.x >> m_position.y >> m_elevation;
    }

    const sf::Vector2f& getPosition() { return m_position; }
    const sf::Vector2f& getOldPosition() { return m_positionOld; }
    unsigned int getElevation() { return m_elevation; }

    void setPosition(const sf::Vector2f& l_vec) {
        m_positionOld = m_position;
        m_position = l_vec;
    }

    void setPosition(float l_x, float l_y) {
        m_positionOld = m_position;
        m_position = sf::Vector2f(l_x, l_y);
    }

    void setElevation(const unsigned int& l_elevation) { m_elevation = l_elevation; }

    void moveBy(const sf::Vector2f& l_vec) {
        m_positionOld = m_position;
        m_position += l_vec;
    }

    void moveBy(float l_x, float l_y) {
        m_positionOld = m_position;
        m_position += sf::Vector2f(l_x, l_y);
    }
private:
    sf::Vector2f m_positionOld;
    sf::Vector2f m_position;
    unsigned int m_elevation;
};