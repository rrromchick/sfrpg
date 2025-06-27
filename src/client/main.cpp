#include "Client.h"

void handlePacket(const PacketID& l_id, sf::Packet& l_packet, Client* l_client) {
    if ((PacketType)l_id == PacketType::Message) {
        std::string message;
        l_packet >> message;
        std::cout << message << std::endl;
    } else if ((PacketType)l_id == PacketType::Register) {
        std::string status;
        l_packet >> status;
        std::cout << status << std::endl;
    } else if ((PacketType)l_id == PacketType::Login) {
        std::string username;
        if (!(l_packet >> username)) {
            std::cout << "Failed to login, error occured." << std::endl;
            return;
        }

        l_client->setUserName(username);

        std::cout << "Logged in successfully." << std::endl;
    } else if ((PacketType)l_id == PacketType::Sql) {
        int size;
        if (!(l_packet >> size) || (size == (int)Network::NullID)) {
            std::cout << "Failed to load messages, error occured." << std::endl;
            return;
        }

        for (size_t i = 0; i < (size_t)size; ++i) {
            std::string sender, receiver, content;

            if (!(l_packet >> sender) || !(l_packet >> receiver) || !(l_packet >> content)) break;

            std::cout << "sender: " << sender << ", receiver: " << receiver << ": " << content << std::endl;
        }
    } else if ((PacketType)l_id == PacketType::ChatMessage) {
        std::string status;
        l_packet >> status;
        std::cout << status << std::endl;
    } else if ((PacketType)l_id == PacketType::Chat) {
        int size;
        if (!(l_packet >> size) || (size == (int)Network::NullID)) {
            std::cout << "Failed to load messages, error occured." << std::endl;
            return;
        }

        for (size_t i = 0; i < (size_t)size; ++i) {
            std::string sender, receiver, content;

            if (!(l_packet >> sender) || !(l_packet >> receiver) || !(l_packet >> content)) break;

            std::cout << sender << ": " << content << std::endl;
        }
    } else if ((PacketType)l_id == PacketType::Disconnect) {
        l_client->disconnect();
    }
}

void commandProcess(Client* l_client) {
    while (l_client->isConnected()) {
        std::string str;
        std::getline(std::cin, str);

        if (str != "") {
            if (str == "!quit") {
                l_client->disconnect();
                break;
            } else if (str == "register") {
                std::string email, username, password, repeatPassword;

                std::cout << "Enter your email address: ";
                std::cin >> email;
                std::cout << "Enter your username: ";
                std::cin >> username;
                std::cout << "Enter your password: ";
                std::cin >> password;
                std::cout << "Repeat your password: ";
                std::cin >> repeatPassword;

                while (repeatPassword != password) {
                    std::cout << "Passwords must be similar! Try again: ";
                    std::cin >> repeatPassword;
                }

                sf::Packet packet;
                stampPacket(PacketType::Register, packet);
                packet << email << username << password << repeatPassword;
                l_client->send(packet);
                continue;
            } else if (str == "login") {
                std::string email, password;

                std::cout << "Enter your email address: ";
                std::cin >> email;
                std::cout << "Enter your password: ";
                std::cin >> password;

                sf::Packet packet;
                stampPacket(PacketType::Login, packet);
                packet << email << password;
                l_client->send(packet);
                continue;
            } else if (str == "chat") {
                sf::Packet packet;
                stampPacket(PacketType::Sql, packet);
                packet << l_client->getUserName();
                l_client->send(packet);
                continue;
            } else if (str == "message") {
                sf::Packet packet;
                stampPacket(PacketType::ChatMessage, packet);

                std::string receiver;
                std::string content;

                std::cout << "To whom: ";
                std::cin >> receiver;
                std::cout << "Enter your message (very important): ";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::getline(std::cin, content);

                packet << l_client->getUserName() << receiver << content;
                l_client->send(packet);
                continue;
            } else if (str == "chatwith") {
                sf::Packet packet;
                stampPacket(PacketType::Chat, packet);

                std::string other;
                std::cout << "Who do you want to talk to? ";
                std::cin >> other;

                packet << l_client->getUserName() << other;
                l_client->send(packet);
                continue;
            }

            sf::Packet packet;
            stampPacket(PacketType::Message, packet);
            packet << str;
            l_client->send(packet);
        }
    }
}

int main(int argc, char* argv[]) {
    sf::IpAddress ip;
    PortNumber port;

    if (argc == 1) {
        std::cout << "Enter server IP: ";
        std::cin >> ip;
        std::cout << "Enter server PORT: ";
        std::cin >> port;
    } else if (argc == 3) {
        ip = argv[1];
        port = atoi(argv[2]);
    } else {
        return 0;
    }

    NetworkProtocol protocol;
    
    std::string protocolStr;
    std::cout << "Enter client protocol (TCP or UDP): ";
    std::cin >> protocolStr;

    if (protocolStr.find("TCP") != std::string::npos) protocol = NetworkProtocol::TCP;
    else protocol = NetworkProtocol::UDP;

    Client client(protocol);
    client.setUp(handlePacket);
    client.setServerInformation(ip, port);

    if (client.connect()) {
        std::thread c(&commandProcess, &client);
        
        sf::Clock clock;
        clock.restart();

        while (client.isConnected()) {
            client.update(clock.restart());
        }

        if (c.joinable()) {
            c.join();
        }
    } else {
        std::cout << "Failed to connect to the server." << std::endl;
    }

    std::cout << "Quitting..." << std::endl;
    return 0;
}