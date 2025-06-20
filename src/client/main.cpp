#include "UdpClient.h"

void HandlePacket(const PacketID& l_id, sf::Packet& l_packet, UdpClient* l_client) {
    if ((PacketType)l_id == PacketType::Message) {
        std::string msg;
        l_packet >> msg;
        std::cout << msg << std::endl;
    } else if ((PacketType)l_id == PacketType::Register) {
        int id;
        if (!(l_packet >> id)) return;
        if (id == (int)Network::NullID) { std::cout << "Failed to sign up!" << std::endl; }
        std::cout << "Signed up successfully." << std::endl;
    } else if ((PacketType)l_id == PacketType::Login) {
        int id;
        if (!(l_packet >> id)) return;
        if (id == (int)Network::NullID) { std::cout << "Failed to log in!" << std::endl; return; }
        UserData data;
        l_packet >> data;
        l_client->SetUserData(data);
        std::cout << "Logged in successfully." << std::endl;
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
            } else if (str == "register") {
                sf::Packet packet;
                StampPacket(PacketType::Register, packet);
                std::string firstName, lastName, password, repeatPassword, email;
                std::cout << "Enter your first name: ";
                std::cin >> firstName;
                std::cout << "Enter your last name: ";
                std::cin >> lastName;
                std::cout << "Enter your password: ";
                std::cin >> password;
                std::cout << "Repeat password: ";
                std::cin >> repeatPassword;
                while (repeatPassword != password) {
                    std::cout << "Passwords differ! Try again." << std::endl;
                    std::cin >> repeatPassword;
                }
                std::cout << "Enter your email address: ";
                std::cin >> email;
                packet << firstName << lastName << password << email;
                l_client->Send(packet);
            } else if (str == "login") {
                sf::Packet packet;
                StampPacket(PacketType::Login, packet);
                std::string firstName, lastName, password, email;
                std::cout << "Enter your first name: ";
                std::cin >> firstName;
                std::cout << "Enter your last name: ";
                std::cin >> lastName;
                std::cout << "Enter your password: ";
                std::cin >> password;
                std::cout << "Enter your email address: ";
                std::cin >> email;
                packet << firstName << lastName << password << email;
                l_client->Send(packet);
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