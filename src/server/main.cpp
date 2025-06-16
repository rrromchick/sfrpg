#include "UdpServer.h"

void HandlePacket(sf::IpAddress &l_ip, const PortNumber &l_port, const PacketID &l_pid, sf::Packet &l_packet, UdpServer *l_server) {
    ClientID cid = l_server->GetClientID(l_ip, l_port);
    if (cid >= 0) {
        if ((PacketType)l_pid == PacketType::Disconnect) {
            l_server->RemoveClient(cid);
            std::string message = "Client " + std::to_string(cid) + " has left.";
            sf::Packet packet;
            StampPacket(PacketType::Message, packet);
            l_server->Broadcast(packet);
        }
        if ((PacketType)l_pid == PacketType::Message) {
            std::string receivedMessage;
            l_packet >> receivedMessage;
            std::string msg = l_ip.toString() + ":" + std::to_string(l_port) + " " + receivedMessage;
            sf::Packet packet;
            StampPacket(PacketType::Message, packet);
            packet << msg;
            l_server->Broadcast(packet, cid);
        }
    } else {
        if ((PacketType)l_pid == PacketType::Connect) {
            ClientID id = l_server->AddClient(l_ip, l_port);
            sf::Packet packet;
            StampPacket(PacketType::Connect, packet);
            l_server->Send(id, packet);
        }
    }
}

void CommandLine(UdpServer *l_server) {
    while (l_server->IsRunning()) {
        std::string str;
        std::getline(std::cin, str);
        if (str == "!q") {
            l_server->Stop();
            return;
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
        while (server.IsRunning()) {
            server.Update(clock.restart());
        }
    }
    return 0;
}   