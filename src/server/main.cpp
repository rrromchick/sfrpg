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
            ClientInfo info(l_ip, l_port, sf::Time::Zero);
            l_server->GetClientInfo(cid, info);
            std::string msg;
            msg = info.m_playerName + ": " + receivedMessage;
            sf::Packet packet;
            StampPacket(PacketType::Message, packet);
            packet << msg;
            l_server->Broadcast(packet, cid);
        } else if ((PacketType)l_id == PacketType::Register) {
            std::string firstName, lastName, password, email;
            if (!(l_packet >> firstName) || !(l_packet >> lastName) || 
                !(l_packet >> password) || !(l_packet >> email))
            { return; }
            int sz = l_server->GetDatabase()->SelectFromUsers(-1, firstName, lastName, "", "").size();
            sz += l_server->GetDatabase()->SelectFromUsers(-1, "", "", "", email).size();
            if (sz > 0) {
                sf::Packet packet;
                StampPacket(PacketType::Register, packet);
                packet << (unsigned int)Network::NullID;
                return;
            } else {
                l_server->GetDatabase()->InsertIntoUsers(l_server->GetUsersCount(), firstName, lastName, password, email);
                sf::Packet packet;
                StampPacket(PacketType::Register, packet);
                packet << l_server->GetUsersCount();
                l_server->Send(cid, packet);
                l_server->IncrementUsersCount();
            }
        } else if ((PacketType)l_id == PacketType::Login) {
            std::string firstName, lastName, password, email;
            if (!(l_packet >> firstName) || !(l_packet >> lastName) || 
                !(l_packet >> password) || !(l_packet >> email))
            { return; }
            DbResult result = l_server->GetDatabase()->SelectFromUsers(-1, firstName, lastName, password, email);
            if (result.size() != 1) {
                sf::Packet packet;
                StampPacket(PacketType::Login, packet);
                packet << (unsigned int)Network::NullID;
                return;
            } else {
                UserData data;
                data.m_id = atoi(result[0].at("ID").c_str());
                data.m_firstName = result[0].at("FIRSTNAME");
                data.m_lastName = result[0].at("LASTNAME");
                data.m_password = result[0].at("PASSWORD");
                data.m_email = result[0].at("EMAIL");
                sf::Packet packet;
                StampPacket(PacketType::Login, packet);
                packet << data.m_id << data;
                l_server->Send(cid, packet);
            }
        }
    } else {
        if ((PacketType)l_id == PacketType::Connect) {
            std::string name;
            l_packet >> name;
            ClientID cid = l_server->AddClient(l_ip, l_port, name);
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