#include "MessageData.h"

sf::Packet& operator <<(sf::Packet& l_packet, const MessageData& l_data) {
    return l_packet << l_data.m_id << l_data.m_timestamp << l_data.m_sender 
        << l_data.m_receiver << l_data.m_content;
}

sf::Packet& operator >>(sf::Packet& l_packet, MessageData& l_data) {
    return l_packet >> l_data.m_id >> l_data.m_timestamp >> l_data.m_sender 
        >> l_data.m_receiver >> l_data.m_content;
}