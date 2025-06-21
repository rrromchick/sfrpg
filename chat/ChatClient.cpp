#include "ChatClient.h"
#include <iostream>
#include <chrono>

ChatClient::ChatClient(ClientProtocol l_protocol, MessageQueue& l_incomingQueue)
    : m_protocol(l_protocol), m_incomingQueue(l_incomingQueue),
    m_running(false), m_connected(false), m_serverPort(0)
{
    if (m_protocol == ClientProtocol::TCP) {
        m_tcpSocket = std::make_unique<sf::TcpSocket>();
        m_tcpSocket->setBlocking(false);
    } else {
        m_udpSocket = std::make_unique<sf::UdpSocket>();
        m_udpSocket->setBlocking(false);
        if (m_udpSocket->bind(sf::Socket::AnyPort) != sf::Socket::Done) {
            std::cerr << "Error: Could not bind UDP client socket to any port." << std::endl;
        }
    }
}

ChatClient::~ChatClient() {
    disconnect();
}

bool ChatClient::connectToServer(const sf::IpAddress& l_ip, unsigned short l_port) {
    m_serverIp = l_ip;
    m_serverPort = l_port;
    m_running = true;

    if (m_protocol == ClientProtocol::TCP) {
        std::cout << "Attempting to connect TCP to " << m_serverIp << ":" << m_serverPort << std::endl;
        if (m_tcpSocket->connect(m_serverIp, m_serverPort, sf::seconds(5)) != sf::Socket::Done) {
            std::cerr << "Error: Could not connect TCP to server " << m_serverIp << ":" << std::to_string(m_serverPort) << std::endl;
            m_running = false;
            return false;
        }
        m_connected = true;
        std::cout << "TCP connected to " << m_serverIp << ":" << m_serverPort << std::endl;
        m_incomingQueue.push("Connected to TCP server: " + m_serverIp.toString() + ":" + std::to_string(m_serverPort));
        m_clientThread = std::thread(&ChatClient::tcpClientLoop, this);
    } else {
        m_connected = true;
        std::cout << "UDP client ready to communicate with " << m_serverIp << ":" << m_serverPort << std::endl;
        m_incomingQueue.push("UDP client ready to communicate with server: " + m_serverIp.toString() + ":" + std::to_string(m_serverPort));
        m_clientThread = std::thread(&ChatClient::udpClientLoop, this);
    }

    return true;
}

void ChatClient::disconnect() {
    if (m_running) {
        m_running = false;
        if (m_protocol == ClientProtocol::TCP && m_connected) {
            m_tcpSocket->disconnect();
            m_connected = false;
        } else if (m_protocol == ClientProtocol::UDP) {
            m_udpSocket->unbind();
            m_connected = false;
        }

        if (m_clientThread.joinable()) {
            m_clientThread.join();
        }
        std::cout << "Client disconnected." << std::endl;
        m_incomingQueue.push("Disconnected from server.");
    }
}

void ChatClient::sendMessage(const std::string& l_message) {
    if (!m_connected) {
        std::cerr << "Not connected to a server. Cannot send message." << std::endl;
        return;
    }

    if (m_protocol == ClientProtocol::TCP) {
        if (m_tcpSocket->send(l_message.c_str(), l_message.length() + 1) != sf::Socket::Done) {
            std::cerr << "Failed sending TCP message." << std::endl;
            m_incomingQueue.push("Error sending TCP message. Disconnecting...");
            disconnect();
        }
    } else {
        if (m_udpSocket->send(l_message.c_str(), l_message.length() + 1, m_serverIp, m_serverPort) != sf::Socket::Done) {
            std::cerr << "Error sending UDP message." << std::endl;
            m_incomingQueue.push("Error sending UDP message.");
        }
    }
}

std::string ChatClient::getStatus() const {
    if (!m_connected) {
        return "Client: Not connected";
    }
    if (m_protocol == ClientProtocol::TCP) {
        return "Client: TCP, connected to " + m_serverIp.toString() + ":" + std::to_string(m_serverPort);
    } else {
        return "Client: UDP, target " + m_serverIp.toString() + ":" + std::to_string(m_serverPort);
    }
}

void ChatClient::tcpClientLoop() {
    char buffer[1024];
    std::size_t received;

    while (m_running && m_connected) {
        sf::Socket::Status status = m_tcpSocket->receive(buffer, sizeof(buffer), received);
        if (status == sf::Socket::Done) {
            buffer[received] = '\0';
            m_incomingQueue.push("[TCP Server]: " + std::string(buffer));
        } else if (status == sf::Socket::Disconnected) {
            std::cerr << "Server disconnected." << std::endl;
            m_incomingQueue.push("Server disconnected.");
            disconnect();
            break;
        } else if (status == sf::Socket::NotReady) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        } else {
            std::cerr << "TCP client receive error: " << status << std::endl;
            m_incomingQueue.push("TCP receive error. Disconnecting...");
            disconnect();
            break;
        }
    }
}

void ChatClient::udpClientLoop() {
    char buffer[1024];
    std::size_t received;
    sf::IpAddress senderIp;
    unsigned short senderPort;

    while (m_running && m_connected) {
        sf::Socket::Status status = m_udpSocket->receive(buffer, sizeof(buffer), received, senderIp, senderPort);

        if (status == sf::Socket::Done) {
            if (senderIp == m_serverIp && senderPort == m_serverPort) {
                buffer[received] = '\0';
                m_incomingQueue.push("[UDP Server]: " + std::string(buffer));
            } else {
                // std::cout << "Received unexpected UDP packet from " << senderIp << ":" << senderPort << std::endl;
            }
        } else if (status == sf::Socket::NotReady) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        } else if (status == sf::Socket::Error) {
            std::cerr << "UDP client receive error" << std::endl;
        }
    }
}