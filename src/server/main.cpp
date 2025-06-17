#include "UdpServer.h"

void HandlePacket(sf::IpAddress& l_ip, const PortNumber& l_port, const PacketID& l_id, sf::Packet& l_packet, UdpServer* l_server) {
    ClientID cid = l_server->GetClientID(l_ip, l_port);
    if (cid >= 0) {
        if ((PacketType)l_id == PacketType::Disconnect) {
            l_server->RemoveClient(cid);
            std::string msg;
            msg = "Client " + std::to_string(cid) + " has left.";
            sf::Packet packet;
            StampPacket(PacketType::Message, packet);
            packet << msg;
            l_server->Broadcast(packet, cid);
        } else if ((PacketType)l_id == PacketType::Message) {
            std::string receivedMessage;
            l_packet >> receivedMessage;
            std::string msg;
            msg = l_ip.toString() + ":" + std::to_string(l_port) + " " + receivedMessage;
            sf::Packet packet;
            StampPacket(PacketType::Message, packet);
            packet << msg;
            l_server->Broadcast(packet, cid);
        }
    } else {
        if ((PacketType)l_id == PacketType::Connect) {
            ClientID cid = l_server->AddClient(l_ip, l_port);
            sf::Packet packet;
            StampPacket(PacketType::Connect, packet);
            l_server->Send(cid, packet);
        }
    }
}

void CommandLine(UdpServer* l_server) {
    while (l_server->IsRunning()) {
        std::string str;
        std::getline(std::cin, str);
        if (str == "!quit") {
            l_server->Stop();
            break;
        } else if (str == "dc") {
            std::cout << "Disconnecting all clients..." << std::endl;
            l_server->DisconnectAll();
        } else if (str == "clients") {
            std::cout << l_server->GetClientCount() << " clients online: " << std::endl;
            std::cout << l_server->GetClientList() << std::endl;
        }
    }
}

int main() {
    UdpServer server(HandlePacket);
    if (server.Start()) {
        sf::Thread c(&CommandLine, &server);
        c.launch();
        sf::Clock clock;
        clock.restart();
        while (server.IsRunning()) {
            server.Update(clock.restart());
        }
    }
    
    std::cout << "Quitting..." << std::endl;
    return 0;
}