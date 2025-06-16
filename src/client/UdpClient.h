#pragma once

#include <SFML/Network.hpp>
#include "PacketTypes.h"
#include "NetworkDefinitions.h"
#include <functional>
#include <iostream>

#define CONNECT_TIMEOUT 5000

class UdpClient;
using PacketHandler = std::function<void(const PacketID&, sf::Packet&, UdpClient*)>;

class UdpClient {
public:
    UdpClient();
    ~UdpClient();

    template <class T>
    void Setup(void(T::*l_handler)(const PacketID&, sf::Packet&, UdpClient*), T *l_instance) {
        m_packetHandler = std::bind(l_handler, l_instance, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3);
    }
    void Setup(void(*l_handler)(const PacketID&, sf::Packet&, UdpClient*));

    bool Send(sf::Packet &l_packet);
    void SetServerInformation(const sf::IpAddress &l_ip, const PortNumber &l_port);

    void Listen();
    void Update(const sf::Time &l_time);
    const sf::Time& GetTime() const;
    const sf::Time& GetLastHeartbeat() const;
    void SetTime(const sf::Time &l_time);

    bool Connect();
    bool Disconnect();

    void SetPlayerName(const std::string &l_name);

    bool IsConnected() const;
    sf::Mutex& GetMutex();
private:
    std::string m_playerName;

    sf::UdpSocket m_socket;
    PacketHandler m_packetHandler;
    sf::IpAddress m_serverIp;
    PortNumber m_serverPort;
    sf::Time m_serverTime;
    sf::Time m_lastHeartbeat;

    bool m_connected;

    sf::Thread m_listenThread;
    sf::Mutex m_mutex;
};