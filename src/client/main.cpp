#include "Client.h"

void handlePacket(const PacketID& l_id, sf::Packet& l_packet, Client* l_client) {
    if ((PacketType)l_id == PacketType::Message) {
        std::string message;
        l_packet >> message;
        std::cout << message << std::endl;
    } else if ((PacketType)l_id == PacketType::Message) {
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