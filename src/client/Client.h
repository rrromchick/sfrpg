#pragma once

#include <SFML/Network.hpp>
#include "NetworkDefinitions.h"
#include "PacketTypes.h"
#include <memory>
#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

#define CONNECT_TIMEOUT 5000

class Client;
using PacketHandler = std::function<void(const PacketID&, sf::Packet&, Client*)>;

class Client {
public:
    Client(const NetworkProtocol& l_protocol);
    ~Client();

    bool connect();
    bool disconnect();
    bool send(sf::Packet& l_packet);

    void tcpListen();
    void udpListen();
    void update(const sf::Time& l_time);

    template <class T>
    void setUp(void(T::*l_handler)(const PacketID&, sf::Packet&, Client*), T* l_instance) {
        m_packetHandler = std::bind(l_handler, l_instance, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3);
    }
    void setUp(void(*l_handler)(const PacketID&, sf::Packet&, Client*));

    void setServerInformation(const sf::IpAddress& l_ip, const PortNumber& l_port);
    void unregisterPacketHandler();

    const sf::Time& getTime() const;
    const sf::Time& getLastHeartbeat() const;
    void setTime(const sf::Time& l_time);

    bool isConnected() const;
    std::mutex& getMutex();

    const std::string& getUserName() const;
    void setUserName(const std::string& l_name);
private:
    NetworkProtocol m_protocol;

    std::string m_username;

    std::unique_ptr<sf::UdpSocket> m_udpSocket;
    std::shared_ptr<sf::TcpSocket> m_tcpSocket;

    sf::Time m_serverTime;
    sf::Time m_lastHeartbeat;

    PacketHandler m_packetHandler;
    std::thread m_clientThread;
    std::mutex m_clientMutex;
    std::atomic_bool m_connected;

    sf::IpAddress m_serverIp;
    PortNumber m_serverPort;
};