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

        if ((PacketType)l_id == PacketType::Register) {
            std::string email, username, password;
            l_packet >> email >> username >> password;

            sf::Packet packet;
            stampPacket(PacketType::Register, packet);

            if (l_server->registerUser(email, username, password)) {
                packet << "Registered successfully!";
            } else {
                packet << "Failed to register. Error occured!";
            };

            l_server->send(cid, packet);
        }

        if ((PacketType)l_id == PacketType::Login) {
            std::string email, password, username;
            l_packet >> email >> password;

            sf::Packet packet;
            stampPacket(PacketType::Login, packet);

            if (l_server->authenticateUser(email, password, username)) {
                packet << username;
            }

            l_server->send(cid, packet);
        }

        if ((PacketType)l_id == PacketType::Sql) {
            std::string username;
            l_packet >> username;

            sf::Packet packet;
            stampPacket(PacketType::Sql, packet);

            DbResult* result = l_server->getMessagesForUser(username);
            if (!result) {
                std::cout << "Failed to load messages, error occured." << std::endl;
                packet << (sf::Int32)Network::NullID;
                return;
            }
            packet << (sf::Int32)result->size();
            
            for (size_t i = 0; i < result->size(); ++i) {
                std::string sender, receiver, content;

                std::tie(sender, receiver, content) = (*result)[i];
                packet << sender << receiver << content;
            }

            l_server->send(cid, packet);
        }

        if ((PacketType)l_id == PacketType::ChatMessage) {
            std::string sender, receiver, content;
            l_packet >> sender >> receiver >> content;

            sf::Packet packet;
            stampPacket(PacketType::ChatMessage, packet);
            
            if (!l_server->addMessage(sender, receiver, content)) {
                packet << "Failed to add message, error occured.";
                l_server->send(cid, packet);
                return;
            }

            packet << "Message has been successfully sent!";
            l_server->send(cid, packet);
        }

        if ((PacketType)l_id == PacketType::Chat) {
            std::string firstUser, secondUser;
            l_packet >> firstUser >> secondUser;

            sf::Packet packet;
            stampPacket(PacketType::Chat, packet);

            DbResult* result = l_server->getChatMessages(firstUser, secondUser);
            if (!result) {
                std::cout << "Failed to load chat messages, error occured." << std::endl;
                packet << (sf::Int32)Network::NullID;
                l_server->send(cid, packet);
                return;
            }

            packet << (sf::Int32)result->size();
            for (size_t i = 0; i < result->size(); ++i) {
                std::string sender, receiver, content;

                std::tie(sender, receiver, content) = (*result)[i];
                packet << sender << receiver << content;
            }

            l_server->send(cid, packet);
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