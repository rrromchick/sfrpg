#include "UdpClient.h"
#include <SFML/Network.hpp>
#include <iostream>

void HandlePacket(const PacketID& l_id, sf::Packet &l_packet, UdpClient *l_client) {
    if ((PacketType)l_id == PacketType::Message) {
        std::string msg;
        l_packet >> msg;
        std::cout << msg << std::endl;
    } else if ((PacketType)l_id == PacketType::Disconnect) {
        l_client->Disconnect();
    }
}

void CommandLine(UdpClient *l_client) {
    while (l_client->IsConnected()) {
        std::string str;
        std::getline(std::cin, str);
        if (str != "") {
            if (str == "!quit") {
                l_client->Disconnect();
                break;
            }
            sf::Packet message;
            StampPacket(PacketType::Message, message);
            message << str;
            l_client->Send(message);
        }
    }
}

int main(int argc, char *argv[])  {
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

    UdpClient client;
    client.SetServerInformation(ip, port);
    client.Setup(HandlePacket);
    sf::Thread c(&CommandLine, &client);

    if (client.Connect()) {
        c.launch();
        sf::Clock clock;
        clock.restart();
        while (client.IsConnected()) {
            client.Update(clock.restart());
        }
    } else {
        std::cout << "Failed to connect!" << std::endl;
    }

    std::cout << "Quitting..." << std::endl;
    return 0;
}