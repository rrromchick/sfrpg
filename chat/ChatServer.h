#pragma once

#include <SFML/Network.hpp>
#include <string>
#include <atomic>
#include <thread>
#include <memory>
#include <unordered_map>
#include <map>
#include <vector>
#include "MessageQueue.hpp"

struct ClientInfo;

enum class ServerProtocol {
    TCP, UDP
};

class ChatServer {
public:
    ChatServer(ServerProtocol l_protocol, MessageQueue* l_incomingQueue);
    ~ChatServer();

    bool start(unsigned short l_port);
    void stop();

    void sendMessage(const std::string& l_message);
    std::string getStatus() const;
private:
    ServerProtocol m_protocol;
    MessageQueue* m_incomingQueue;

    std::unique_ptr<sf::TcpListener> m_tcpListener;
    std::unique_ptr<sf::UdpSocket> m_udpSocket;

    std::vector<std::shared_ptr<sf::TcpSocket>> m_tcpClients;
    sf::SocketSelector m_selector;
    std::map<unsigned short, std::shared_ptr<sf::TcpSocket>> m_tcpClientMap;

    sf::IpAddress m_lastUdpClientIp;
    unsigned short m_lastUdpClientPort;

    std::thread m_serverThread;
    std::atomic<bool> m_running;
    unsigned short m_port;

    std::mutex m_clientMutex;

    void tcpServerLoop();
    void udpServerLoop();

    void handleTcpClientReceive(std::shared_ptr<sf::TcpSocket> l_client);
    bool sendTcpMessage(std::shared_ptr<sf::TcpSocket> l_client, const std::string& l_message);
};