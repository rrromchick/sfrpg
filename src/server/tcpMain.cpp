#include "TcpServer.h"

void HandlePacket(sf::TcpSocket* l_socket, const PacketID& l_id, sf::Packet& l_packet, TcpServer* l_server) {
    ClientID id = l_server->GetClientID(l_socket);
    if (id >= 0) {
        if ((PacketType)l_id == PacketType::Disconnect) {
            l_server->RemoveClient(id);
            std::string msg;
            msg = "Client left " + std::to_string(l_socket->getLocalPort());

            sf::Packet packet;
            StampPacket(PacketType::Message, packet);
            packet << msg;
            l_server->Broadcast(packet, id);
        } else if ((PacketType)l_id == PacketType::Message) {
            std::string receivedMsg;
            l_packet >> receivedMsg;
            std::string msg;
            msg = std::to_string(l_socket->getLocalPort()) + ":" + receivedMsg;

            sf::Packet packet;
            StampPacket(PacketType::Message, packet);
            packet << msg;
            l_server->Broadcast(packet, id);
        }
    } else {
        if ((PacketType)l_id == PacketType::Connect) {
            ClientID cid = l_server->AddClient(l_socket);
            sf::Packet packet;
            StampPacket(PacketType::Connect, packet);
            l_server->Send(cid, packet);
        }
    }
}

void CommandLine(TcpServer* l_server) {
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
            std::cout << l_server->GetClientCount() << " clients online:" << std::endl;
            std::cout << l_server->GetClientList() << std::endl;
        }
    }
}

int main() {
    TcpServer server(HandlePacket);

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