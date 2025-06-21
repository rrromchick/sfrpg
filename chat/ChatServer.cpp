#include "ChatServer.h"
#include <iostream>
#include <chrono>
#include <utility>

ChatServer::ChatServer(ServerProtocol l_protocol, MessageQueue& l_incomingQueue)
    : m_protocol(l_protocol), m_incomingQueue(l_incomingQueue),
    m_running(false), m_lastUdpClientPort(0), m_port(0)
{
    if (m_protocol == ServerProtocol::TCP) {
        m_tcpListener = std::make_unique<sf::TcpListener>();
    } else {
        m_udpSocket = std::make_unique<sf::UdpSocket>();
        m_udpSocket->setBlocking(false);
    }
}

ChatServer::~ChatServer() {
    stop();
}

bool ChatServer::start(unsigned short l_port) {
    m_port = l_port;
    m_running = true;

    if (m_protocol == ServerProtocol::TCP) {
        if (m_tcpListener->listen(m_port) != sf::Socket::Done) {
            std::cerr << "Error: Couls not bind TCP listener to port " << m_port << std::endl;
            m_running = false;
            return false;
        }
        std::cout << "TCP Server started, listening on port " << m_port << std::endl;
        m_selector.add(*m_tcpListener);
        m_serverThread = std::thread(&ChatServer::tcpServerLoop, this);
    } else {
        if (m_udpSocket->bind(m_port) != sf::Socket::Done){
            std::cerr << "Error: Could not bind UDP socket to port " << m_port << std::endl;
            m_running = false;
            return false;
        }
        std::cout << "UDP Server started, listening on port " << m_port << std::endl;
        m_serverThread = std::thread(&ChatServer::udpServerLoop, this);
    }

    return true;
}

void ChatServer::stop() {
    if (m_running) {
        m_running = false;

        if (m_protocol == ServerProtocol::TCP) {
            m_tcpListener->close();
            
            std::lock_guard<std::mutex> lock(m_clientMutex);
            for (const auto& pair : m_tcpClientMap) {
                pair.second->disconnect();
            }
            m_tcpClientMap.clear();
            m_selector.clear();
        } else {
            m_udpSocket->unbind();
        }
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }

    std::cout << "Server stopped." << std::endl;
}

void ChatServer::sendMessage(const std::string& l_message) {
    if (!m_running) {
        std::cerr << "Server is not running. Cannot send message." << std::endl;
        return;
    }

    if (m_protocol == ServerProtocol::TCP) {
        std::lock_guard<std::mutex> lock(m_clientMutex);

        for (const auto& pair : m_tcpClientMap) {
            if (!sendTcpMessage(pair.second, l_message)) {
                std::cerr << "Failed to send message to a TCP client." << std::endl;
            }
        }
    } else {
        if (m_lastUdpClientIp != sf::IpAddress::None && m_lastUdpClientPort != 0) {
            if (m_udpSocket->send(l_message.c_str(), l_message.length() + 1, m_lastUdpClientIp, m_lastUdpClientPort) != sf::Socket::Done) {
                std::cerr << "Error sending UDP message to " << m_lastUdpClientIp << ":" << m_lastUdpClientPort << std::endl;
            } else {
                // std::cout << "message sent successfully." << std::endl;
            }
        } else {
            std::cerr << "No UDP client has sent a message yet to respond to." << std::endl;
        }
    }
}

std::string ChatServer::getStatus() const {
    if (!m_running) {
        return "Server: Not Running";
    }
    if (m_protocol == ServerProtocol::TCP) {
        return "Server: TCP, port " + std::to_string(m_port) + ", clients: " + std::to_string(m_tcpClientMap.size());
    } else {
        return "Server: UDP, port " + std::to_string(m_port);
    }
}

void ChatServer::tcpServerLoop() {
    m_tcpListener->setBlocking(false);

    while (m_running) {
        if (m_selector.wait(sf::milliseconds(100))) {
            if (m_selector.isReady(*m_tcpListener)) {
                auto newClientSocket = std::make_shared<sf::TcpSocket>();
                if (m_tcpListener->accept(*newClientSocket) == sf::Socket::Done) {
                    newClientSocket->setBlocking(false);
                    m_selector.add(*newClientSocket);

                    std::lock_guard<std::mutex> lock(m_clientMutex);

                    unsigned short clientPort = newClientSocket->getRemotePort();
                    m_tcpClientMap[clientPort] = newClientSocket;
                    m_incomingQueue.push("New TCP client connected from " + newClientSocket->getRemoteAddress().toString() + ":" + std::to_string(clientPort));
                    std::cout << "New TCP client connected from " << newClientSocket->getRemoteAddress().toString() + ":" << std::to_string(clientPort) << std::endl;
                } else {
                    std::cerr << "Error accepting new TCP connection." << std::endl;
                }
            }

            std::vector<unsigned short> clientsToRemove;
            std::lock_guard<std::mutex> lock(m_clientMutex);
            for (const auto& pair : m_tcpClientMap) {
                unsigned short clientId = pair.first;
                std::shared_ptr<sf::TcpSocket> client = pair.second;

                if (m_selector.isReady(*client)) {
                    char buffer[1024];
                    std::size_t received;
                    if (client->receive(buffer, sizeof(buffer), received) == sf::Socket::Done) {
                        buffer[received] = '\0';
                        std::string msg = buffer;
                        m_incomingQueue.push("[TCP]" + client->getRemoteAddress().toString() + std::to_string(client->getRemotePort()) + ": " + msg);

                        for (const auto& other_pair : m_tcpClientMap) {
                            if (other_pair.first != clientId) {
                                if (!sendTcpMessage(other_pair.second, "[From " + std::to_string(clientId) + "] " + msg)) {
                                    std::cerr << "Failed to echo message to client " << other_pair.first << std::endl;
                                }
                            }
                        }
                    } else if (client->receive(buffer, sizeof(buffer), received) == sf::Socket::Disconnected) {
                        std::cout << "TCP client disconnected: " << client->getRemoteAddress().toString() << ":" << std::to_string(client->getRemotePort()) << std::endl;
                        m_incomingQueue.push("TCP client disconnected: " + client->getRemoteAddress().toString() + ":" + std::to_string(client->getRemotePort()));
                        clientsToRemove.push_back(clientId);
                        m_selector.remove(*client);
                    } else {
                        // Other error or no data yet, continue
                    }
                }
            }

            for (unsigned short id : clientsToRemove) {
                m_tcpClientMap.erase(id);
            }
        }
    }
}

void ChatServer::udpServerLoop() {
    char buffer[1024];
    std::size_t received;
    sf::IpAddress remoteIp;
    unsigned short remotePort;

    while (m_running) {
        sf::Socket::Status status = m_udpSocket->receive(buffer, sizeof(buffer), received, remoteIp, remotePort);

        if (status == sf::Socket::Done) {
            buffer[received] = '\0';
            m_lastUdpClientIp = remoteIp;
            m_lastUdpClientPort = remotePort;
            m_incomingQueue.push("[UDP] " + remoteIp.toString() + ":" + std::to_string(remotePort) + ": " + buffer);
        } else if (status == sf::Socket::NotReady) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        } else if (status == sf::Socket::Error) {
            std::cerr << "Error receiving UDP message." << std::endl;
        }
    }
}

bool ChatServer::sendTcpMessage(std::shared_ptr<sf::TcpSocket> l_client, const std::string& l_message) {
    if (l_client->send(l_message.c_str(), l_message.length() + 1) != sf::Socket::Done) {
        std::cerr << "Failed to send TCP message to " << l_client->getRemoteAddress().toString() << ":" << std::to_string(l_client->getRemotePort()) << std::endl;
        return false;
    }
    return true;
}