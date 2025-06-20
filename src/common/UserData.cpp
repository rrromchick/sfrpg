#include "UserData.h"

sf::Packet& operator <<(sf::Packet& l_packet, const UserData& l_data) {
    return l_packet << l_data.m_id << l_data.m_firstName << l_data.m_lastName   
        << l_data.m_password << l_data.m_email;
}

sf::Packet& operator >>(sf::Packet& l_packet, UserData& l_data) {
    return l_packet >> l_data.m_id >> l_data.m_firstName >> l_data.m_lastName 
        >> l_data.m_password >> l_data.m_email;
}