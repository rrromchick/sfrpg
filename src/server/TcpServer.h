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
    ClientInfo(sf::TcpSocket* l_socket, const sf::Time& l_time) 
        : m_socket(l_socket), m_clientIP(l_socket->getRemoteAddress()), m_clientPORT((PortNumber)l_socket->getRemotePort()), 
        m_lastHeartbeat(l_time), m_heartbeatWaiting(false), m_heartbeatRetry(0), m_ping(0)
    {}

    ClientInfo& operator =(const ClientInfo& l_rhs) {
        m_socket = l_rhs.m_socket;
        m_clientIP = l_rhs.m_clientIP;
        m_clientPORT = l_rhs.m_clientPORT;
        m_lastHeartbeat = l_rhs.m_lastHeartbeat;
        m_heartbeatSent = l_rhs.m_heartbeatSent;
        m_heartbeatWaiting = l_rhs.m_heartbeatWaiting;
        m_heartbeatRetry = l_rhs.m_heartbeatRetry;
        m_ping = l_rhs.m_ping;

        return *this;
    }

    sf::TcpSocket* m_socket;
    sf::IpAddress m_clientIP;
    PortNumber m_clientPORT;
    sf::Time m_lastHeartbeat;
    sf::Time m_heartbeatSent;
    bool m_heartbeatWaiting;
    unsigned short m_heartbeatRetry;
    unsigned int m_ping;
};

using Clients = std::unordered_map<ClientID, ClientInfo>;
class TcpServer;
using PacketHandler = std::function<void(sf::TcpSocket*, const PacketID&, sf::Packet&, TcpServer*)>;
using TimeoutHandler = std::function<void(const ClientID&)>;

class TcpServer {
public:
    template <class T>
    TcpServer(void(T::*l_handler)(sf::TcpSocket*, const PacketID&, sf::Packet&, TcpServer*), T* l_instance)
        : m_listenThread(&TcpServer::Listen, this), m_running(false)
    {
        m_packetHandler = std::bind(l_handler, l_instance, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    }
    TcpServer(void(*l_handler)(sf::TcpSocket*, const PacketID&, sf::Packet&, TcpServer*));
    ~TcpServer();

    bool Start();
    bool Stop();
    bool Send(const ClientID& l_client, sf::Packet& l_packet);
    bool Send(sf::IpAddress& l_ip, const PortNumber& l_port, sf::Packet& l_packet);
    void Broadcast(sf::Packet& l_packet, const ClientID& l_ignore = (ClientID)Network::NullID);
    void DisconnectAll();

    void Listen();
    void Update(const sf::Time& l_time);

    template <class T>
    void BindTimeoutHandler(void(T::*l_handler)(const ClientID&), T* l_instance) {
        m_timeoutHandler = std::bind(l_handler, l_instance, std::placeholders::_1);
    }
    void BindTimeoutHandler(void(*l_handler)(const ClientID&));

    ClientID AddClient(sf::TcpSocket* l_socket);
    ClientID GetClientID(sf::TcpSocket* l_socket);
    bool HasClient(const ClientID& l_client);
    bool HasClient(sf::TcpSocket* l_socket);
    bool RemoveClient(sf::TcpSocket* l_socket);
    bool RemoveClient(const ClientID& l_client);
    bool GetClientInfo(const ClientID& l_client, ClientInfo& l_info);

    unsigned int GetClientCount();
    bool IsRunning() const;
    std::string GetClientList();
    void Setup();

    sf::Mutex& GetMutex();
private:
    sf::SocketSelector m_selector;
    sf::TcpListener m_listener;
    PacketHandler m_packetHandler;
    TimeoutHandler m_timeoutHandler;
    Clients m_clients;
    sf::Time m_serverTime;

    bool m_running;
    unsigned int m_lastID;

    sf::Thread m_listenThread;
    sf::Mutex m_mutex;

    size_t m_totalSent;
    size_t m_totalReceived;
};