#pragma once

#include <SFML/Network.hpp>
#include "PacketTypes.h"
#include "NetworkDefinitions.h"
#include <functional>
#include <iostream>

#define CONNECT_TIMEOUT 5000

class TcpClient;
using PacketHandler = std::function<void(const PacketID&, sf::Packet&, TcpClient*)>;

class TcpClient {
public:
    TcpClient();
    ~TcpClient();

    bool Connect();
    bool Disconnect();

    void Listen();
    bool Send(sf::Packet& l_packet);

    template <class T>
    void Setup(void(T::*l_handler)(const PacketID&, sf::Packet, TcpClient*), T* l_instance) {
        m_packetHandler = std::bind(l_handler, l_instance, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3);
    }
    void Setup(void(*l_handler)(const PacketID&, sf::Packet&, TcpClient*));

    void Update(const sf::Time& l_time);

    const sf::Time& GetTime() const;
    const sf::Time& GetLastHeartbeat() const;
    void SetTime(const sf::Time& l_time);
    void SetServerInformation(const sf::IpAddress& l_ip, const PortNumber& l_port);
    void SetPlayerName(const std::string& l_name);

    bool IsConnected() const;
    void UnregisterPacketHandler();

    sf::Mutex& GetMutex();
private:
    std::string m_playerName;

    sf::TcpSocket m_socket;
    sf::IpAddress m_serverIp;
    PortNumber m_serverPort;
    PacketHandler m_packetHandler;
    sf::Time m_serverTime;
    sf::Time m_lastHeartbeat;

    bool m_connected;
    sf::Thread m_listenThread;
    sf::Mutex m_mutex;
}; 