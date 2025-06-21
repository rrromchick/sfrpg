#pragma once

#include <SFML/Network.hpp>
#include <string>
#include <thread>
#include <memory>
#include <atomic>
#include "MessageQueue.hpp"

enum class ClientProtocol {
    TCP, UDP
};

class ChatClient {
public:
    ChatClient(ClientProtocol l_protocol, MessageQueue& l_incomingQueue);
    ~ChatClient();

    bool connectToServer(const sf::IpAddress& l_ip, unsigned short l_port);
    void disconnect();

    void sendMessage(const std::string& l_message);
    std::string getStatus() const;
private:
    ClientProtocol m_protocol;
    MessageQueue& m_incomingQueue;

    std::unique_ptr<sf::TcpSocket> m_tcpSocket;
    std::unique_ptr<sf::UdpSocket> m_udpSocket;

    sf::IpAddress m_serverIp;
    unsigned short m_serverPort;

    std::thread m_clientThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_connected;

    void tcpClientLoop();
    void udpClientLoop();
};