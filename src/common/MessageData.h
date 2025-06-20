#pragma once

#include <SFML/Network/Packet.hpp>
#include <string>

struct MessageData {
    int m_id;
    std::string m_timestamp;
    int m_sender;
    int m_receiver;
    std::string m_content;
};

sf::Packet& operator <<(sf::Packet& l_packet, const MessageData& l_data);
sf::Packet& operator >>(sf::Packet& l_packet, MessageData& l_data);