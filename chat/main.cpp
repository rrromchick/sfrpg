#include <iostream>
#include <string>
#include <limits>
#include <memory>
#include <thread>
#include <chrono>
#include "MessageQueue.hpp"
#include "ChatServer.h"
#include "ChatClient.h"

void clearInputBuffer() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

unsigned short getPortInput() {
    unsigned short port;
    while (true) {
        std::cout << "Enter port (1024-65535, default 50000): ";
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) {
            return 50000;
        }

        try {
            int p = std::stoi(input);
            if (p >= 1024 && p <= 65535) {
                port = static_cast<unsigned short>(p);
                break;
            } else {
                std::cout << "Invalid port. Please enter a number between 1024 and 65535" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Invalid input. Please enter a valid number." << std::endl;
        }
    }

    return port;
}

sf::IpAddress getIpAddressInput() {
    sf::IpAddress ip;
    while (true) {
        std::cout << "Enter server IP address (e.g., 127.0.0.1 or 'local'): ";
        std::string ipString;
        std::getline(std::cin, ipString);

        if (ipString == "local") {
            ip = sf::IpAddress::LocalHost;
            break;
        }

        ip = sf::IpAddress(ipString);
        if (ip != sf::IpAddress::None) {
            break;
        } else {
            std::cout << "Invalid IP address. Please try again." << std::endl;
        }
    }

    return ip;
}

int main() {
    MessageQueue incomingMessageQueue;

    std::unique_ptr<ChatServer> server = nullptr;
    std::unique_ptr<ChatClient> client = nullptr;

    int choice = -1;
    while (choice != 0) {
        system("cls || clear");

        std::cout << "--- SFML Console Chat ---" << std::endl;
        std::cout << "1. Start TCP Server" << std::endl;
        std::cout << "2. Start UDP Server" << std::endl;
        std::cout << "3. Connect as TCP Client" << std::endl;
        std::cout << "4. Connect as UDP Client" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "------------------------" << std::endl;

        if (server) {
            std::cout << server->getStatus() << std::endl;
        } else if (client) {
            std::cout << client->getStatus() << std::endl;
        } else {
            std::cout << "Status: Not running." << std::endl;
        }

        std::cout << "------------------------" << std::endl;

        std::cout << "Enter your choice: ";
        std::string choiceStr;
        std::getline(std::cin, choiceStr);
        try {
            choice = std::stoi(choiceStr);
        } catch (const std::exception&) {
            choice = -1;
        }

        if (server) {
            server->stop();
            server.reset();
        }
        if (client) {
            client->disconnect();
            client.reset();
        }

        unsigned short port;
        sf::IpAddress ipAddress;

        switch (choice) {
            case 1: {
                port = getPortInput();
                server = std::make_unique<ChatServer>(ServerProtocol::TCP, incomingMessageQueue);
                if (!server->start(port)) {
                    server.reset();
                }
                break;
            }
            case 2: {
                port = getPortInput();
                server = std::make_unique<ChatServer>(ServerProtocol::UDP, incomingMessageQueue);
                if (!server->start(port)) {
                    server.reset();
                }
                break;
            }
            case 3: {
                ipAddress = getIpAddressInput();
                port = getPortInput();
                client = std::make_unique<ChatClient>(ClientProtocol::TCP, incomingMessageQueue);
                if (!client->connectToServer(ipAddress, port)) {
                    client.reset();
                }
            }
            case 4: {
                ipAddress = getIpAddressInput();
                port = getPortInput();
                client = std::make_unique<ChatClient>(ClientProtocol::UDP, incomingMessageQueue);
                if (!client->connectToServer(ipAddress, port)) {
                    client.reset();
                }
                break;
            }
            case 0: {
                std::cout << "Exiting chat application." << std::endl;
                break;
            }
            default: {
                std::cout << "Invalid choice. Please try again." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                break;
            }
        }

        if (server || client) {
            std::cout << "\n--- Chat Mode (Type 'exit' to return to menu) ---" << std::endl;
            std::cout << "Your messages will be sent. Incoming messages will appear above." << std::endl;
            std::string userMessage;
            while (true) {
                while (!incomingMessageQueue.isEmpty()) {
                    std::cout << incomingMessageQueue.pop() << std::endl;
                }

                if ((server && server->getStatus().find("Not Running") != std::string::npos) ||
                    (client && client->getStatus().find("Not Connected") != std::string::npos))
                {
                    std::cout << "Network connection lost. Returning to main menu." << std::endl;
                    break;
                }
                
                std::cout << "> ";
                std::getline(std::cin, userMessage);

                if (userMessage == "exit") {
                    break;
                }

                if (server) {
                    server->sendMessage(userMessage);
                } else if (client) {
                    client->sendMessage(userMessage);
                }
            }
        }
    }

    if (server) {
        server->stop();
    }
    if (client) {
        client->disconnect();
    }
    incomingMessageQueue.stop();

    return 0;
}