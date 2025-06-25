#include "Server.h"

void handlePacket(sf::IpAddress& l_ip, const PortNumber& l_port, const PacketID& l_id, sf::Packet& l_packet, Server* l_server) {
    ClientID cid = l_server->getClientID(l_ip, l_port);
  
    if (cid >= 0) {
        if ((PacketType)l_id == PacketType::Disconnect) {
            l_server->removeClient(cid);
            std::string msg;
            msg = "Client " + std::to_string(cid) + " has left.";
            sf::Packet packet;
            stampPacket(PacketType::Message, packet);
            packet << msg;
            l_server->broadcast(packet, cid);
        }

        if ((PacketType)l_id == PacketType::Message) {
            std::string receivedMsg;
            l_packet >> receivedMsg;
            std::string msg = l_ip.toString() + ":" + std::to_string(l_port) + " " + receivedMsg;
            sf::Packet packet;
            stampPacket(PacketType::Message, packet);
            packet << msg;
            l_server->broadcast(packet, cid);
        }
    } else {
        if ((PacketType)l_id == PacketType::Connect) {
            ClientID id = l_server->addClient(NetworkProtocol::UDP, l_ip, l_port);
            sf::Packet packet;
            stampPacket(PacketType::Connect, packet);
            l_server->send(id, packet);
        }
    }
}

void commandProcess(Server* l_server) {
    while (l_server->isRunning()) {
        std::string str;
        std::getline(std::cin, str);
        
        if (str == "!quit") {
            l_server->stop();
            break;
        } else if (str == "dc") {
            std::cout << "disconnecting all clients..." << std::endl;
            l_server->disconnectAll();
        } else if (str == "clients") {
            std::cout << l_server->getClientCount() << " clients online: " << std::endl;
            std::cout << l_server->getClientList() << std::endl;
        }
    }
}   

int main() {
    Server server(handlePacket);
   
    if (server.start()) {
        std::thread c(&commandProcess, &server);
        
        sf::Clock clock;
        clock.restart();

        while (server.isRunning()) {
            server.update(clock.restart());
        }

        if (c.joinable()) {
            c.join();
        }
    }

    std::cout << "Quitting..." << std::endl;
    return 0;
}