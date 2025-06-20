#pragma once

#include <SFML/Network/Packet.hpp>
#include <string>

struct UserData {
    int m_id;
    std::string m_firstName;
    std::string m_lastName;
    std::string m_password;
    std::string m_email;
};

sf::Packet& operator <<(sf::Packet& l_packet, const UserData& l_data);
sf::Packet& operator >>(sf::Packet& l_packet, UserData& l_data);