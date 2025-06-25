#include "PacketTypes.h"

void stampPacket(const PacketType& l_type, sf::Packet& l_packet) {
    l_packet << (PacketID)l_type;
}