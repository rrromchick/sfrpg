#pragma once

#include <SFML/Network/Packet.hpp>

using PacketID = sf::Int8;

enum class PacketType {
    Disconnect = -1, Connect, Heartbeat, Message, Snapshot, 
    Hurt, Player_Update, Register, Login, Sql, OutOfBounds 
};

void stampPacket(const PacketType& l_type, sf::Packet& l_packet);