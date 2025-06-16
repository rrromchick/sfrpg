#pragma once

#include <SFML/Network.hpp>
#include "PacketTypes.h"
#include "NetworkDefinitions.h"
#include <unordered_map>
#include <functional>
#include <iostream>

#define HEARTBEAT_INTERVAL 1000
#define HEARTBEAT_RETRIES 5

struct ClientInfo {
    ClientInfo(const sf::IpAddress &l_ip, const PortNumber &l_port, const sf::Time &l_time)
        : m_clientIP(l_ip), m_clientPORT(l_port), m_lastHeartbeat(l_time),
        m_heartbeatWaiting(false), m_heartbeatRetry(0), m_ping(0)
    {}

    ClientInfo& operator =(const ClientInfo &l_rhs) {
        m_clientIP = l_rhs.m_clientIP;
        m_clientPORT = l_rhs.m_clientPORT;
        m_lastHeartbeat = l_rhs.m_lastHeartbeat;
        m_heartbeatSent = l_rhs.m_heartbeatSent;
        m_heartbeatWaiting = l_rhs.m_heartbeatWaiting;
        m_heartbeatRetry = l_rhs.m_heartbeatRetry;
        m_ping = l_rhs.m_ping;

        return *this;
    }

    sf::IpAddress m_clientIP;
    PortNumber m_clientPORT;
    sf::Time m_lastHeartbeat;
    sf::Time m_heartbeatSent;
    bool m_heartbeatWaiting;
    unsigned short m_heartbeatRetry;
    unsigned int m_ping;
};

using Clients = std::unordered_map<ClientID, ClientInfo>;
class UdpServer;
using PacketHandler = std::function<void(sf::IpAddress&, const PortNumber&, const PacketID&, sf::Packet&, UdpServer*)>;
using TimeoutHandler = std::function<void(const ClientID&)>;

class UdpServer {
public:
    template <class T>
    UdpServer(void(T::*l_handler)(sf::IpAddress&, const PortNumber&, const PacketID&, sf::Packet&, UdpServer*),
        T *l_instance) : m_listenThread(&UdpServer::Listen, this), m_running(false)
    {
        m_packetHandler = std::bind(l_handler, l_instance, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
    }
    UdpServer(void(*l_handler)(sf::IpAddress&, const PortNumber&, const PacketID&, sf::Packet&, UdpServer*));
    ~UdpServer();

    template <class T>
    void BindTimeoutHandler(void(T::*l_handler)(const ClientID&), T *l_instance) {
        m_timeoutHandler = std::bind(l_handler, l_instance, std::placeholders::_1);
    }
    void BindTimeoutHandler(void(*l_handler)(const ClientID&));

    bool Start();
    bool Stop();
    void DisconnectAll();
    void Listen();
    void Update(const sf::Time &l_time);

    bool Send(sf::IpAddress &l_ip, const PortNumber &l_port, sf::Packet &l_packet);
    bool Send(const ClientID &l_client, sf::Packet &l_packet);
    void Broadcast(sf::Packet &l_packet, const ClientID &l_ignore = (ClientID)Network::NullID);

    bool AddClient(const sf::IpAddress &l_ip, const PortNumber &l_port);
    bool HasClient(const sf::IpAddress &l_ip, const PortNumber &l_port);
    bool HasClient(const ClientID &l_client);
    ClientID GetClientID(const sf::IpAddress &l_ip, const PortNumber &l_port);
    bool RemoveClient(const sf::IpAddress &l_ip, const PortNumber &l_port);
    bool RemoveClient(const ClientID &l_client);
    bool GetClientInfo(const ClientID &l_client, ClientInfo &l_info);

    unsigned int GetClientCount();
    std::string GetClientList();
    void Setup();
    bool IsRunning() const;

    sf::Mutex& GetMutex();
private:
    sf::UdpSocket m_incoming;
    sf::UdpSocket m_outgoing;
    sf::Time m_serverTime;
    
    PacketHandler m_packetHandler;
    TimeoutHandler m_timeoutHandler;

    sf::Thread m_listenThread;
    sf::Mutex m_mutex;

    Clients m_clients;
    unsigned int m_lastID;
    bool m_running;

    size_t m_totalSent;
    size_t m_totalReceived;
};