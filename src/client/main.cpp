#include "UdpClient.h"

void HandlePacket(const PacketID& l_id, sf::Packet& l_packet, UdpClient* l_client) {
    if ((PacketType)l_id == PacketType::Message) {
        std::string msg;
        l_packet >> msg;
        std::cout << msg << std::endl;
    } else if ((PacketType)l_id == PacketType::Disconnect) {
        l_client->Disconnect();
    }
}

void CommandLine(UdpClient* l_client) {
    while (l_client->IsConnected()) {
        std::string str;
        std::getline(std::cin, str);
        
        if (str != "") {
            if (str == "!quit") {
                l_client->Disconnect();
                break;
            }

            sf::Packet packet;
            StampPacket(PacketType::Message, packet);
            packet << str;
            l_client->Send(packet);
        }
    }
}

int main(int argc, char* argv[]) {
    sf::IpAddress ip;
    PortNumber port;
    std::string playerName;

    if (argc == 1) {
        std::cout << "Enter server IP: ";
        std::cin >> ip;
        std::cout << "Enter server PORT: ";
        std::cin >> port;
        std::cout << "Enter name: ";
        std::cin >> playerName;
    } else if (argc == 3) {
        ip = argv[1];
        port = atoi(argv[2]);
        std::cout << "Enter name: ";
        std::cin >> playerName;
    } else {
        return 0;
    }

    UdpClient client;
    client.Setup(HandlePacket);
    client.SetServerInformation(ip, port);
    client.SetPlayerName(playerName);

    sf::Thread c(&CommandLine, &client);

    if (client.Connect()) {
        c.launch();
        sf::Clock clock;
        clock.restart();
        while (client.IsConnected()) {
            client.Update(clock.restart());
        }
    } else {
        std::cout << "Failed to connect to the server" << std::endl;
    }

    std::cout << "Quitting..." << std::endl;
    sf::sleep(sf::milliseconds(1));

    return 0;
}