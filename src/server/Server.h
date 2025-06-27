#pragma once

#include <SFML/Network.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include "NetworkDefinitions.h"
#include "PacketTypes.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>
#include <atomic>
#include <iostream>

#define HEARTBEAT_INTERVAL 1000
#define HEARTBEAT_RETRIES 5

struct ClientInfo {
    ClientInfo(const NetworkProtocol& l_protocol, const sf::IpAddress& l_ip, const PortNumber& l_port, const sf::Time& l_time)
        : m_protocol(l_protocol), m_clientIP(l_ip), m_clientPORT(l_port), m_lastHeartbeat(l_time),
        m_heartbeatWaiting(false), m_heartbeatRetry(0), m_ping(0)
    {}

    ClientInfo& operator =(const ClientInfo& l_rhs) {
        m_protocol = l_rhs.m_protocol;
        m_clientIP = l_rhs.m_clientIP;
        m_clientPORT = l_rhs.m_clientPORT;
        m_lastHeartbeat = l_rhs.m_lastHeartbeat;
        m_heartbeatSent = l_rhs.m_heartbeatSent;
        m_heartbeatWaiting = l_rhs.m_heartbeatWaiting;
        m_ping = l_rhs.m_ping;

        return *this;
    }

    NetworkProtocol m_protocol;
    sf::IpAddress m_clientIP;
    PortNumber m_clientPORT;
    sf::Time m_lastHeartbeat;
    sf::Time m_heartbeatSent;
    bool m_heartbeatWaiting;
    unsigned short m_heartbeatRetry;
    unsigned int m_ping;
};

class Server;
using PacketHandler = std::function<void(sf::IpAddress&, const PortNumber&, const PacketID&, sf::Packet&, Server*)>;
using TimeoutHandler = std::function<void(const ClientID&)>;

using DbResult = std::vector<std::tuple<std::string, std::string, std::string>>;

class Server {
public:
    template <class T>
    Server(void(T::*l_handler)(sf::IpAddress&, const PortNumber&, const PacketID&, sf::Packet&, Server*), T* l_instance)
        : m_tcpThread(&Server::tcpListen, this), m_udpThread(&Server::udpListen, this), m_running(false)
    {
        m_packetHandler = std::bind(l_handler, l_instance, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
    }
    Server(void(*l_handler)(sf::IpAddress&, const PortNumber&, const PacketID&, sf::Packet&, Server*));
    ~Server();

    bool start();
    bool stop();
    bool send(const ClientID& l_client, sf::Packet& l_packet);
    bool send(sf::IpAddress& l_ip, const PortNumber& l_port, sf::Packet& l_packet);
    void broadcast(sf::Packet& l_packet, const ClientID& l_ignore = (ClientID)Network::NullID);
    void disconnectAll();

    void tcpListen();
    void udpListen();
    void update(const sf::Time& l_time);

    template <class T>
    void bindTimeoutHandler(void(T::*l_handler)(const ClientID&), T* l_instance) {
        m_timeoutHandler = std::bind(l_handler, l_instance, std::placeholders::_1);
    }
    void bindTimeoutHandler(void(*l_handler)(const ClientID&));

    ClientID addClient(const NetworkProtocol& l_protocol, const sf::IpAddress& l_ip, const PortNumber& l_port);
    ClientID getClientID(const sf::IpAddress& l_ip, const PortNumber& l_port);
    bool getClientInfo(const ClientID& l_client, ClientInfo& l_info);
    bool hasClient(const ClientID& l_client);
    bool hasClient(const sf::IpAddress& l_ip, const PortNumber& l_port);
    bool removeClient(const ClientID& l_client);
    bool removeClient(const sf::IpAddress& l_ip, const PortNumber& l_port);

    bool initDatabase(const std::string& l_dbPath);
    bool registerUser(const std::string& l_email, const std::string& l_username, const std::string& l_password);
    bool authenticateUser(const std::string& l_email, const std::string& l_password, std::string& l_username);
    bool addMessage(const std::string& l_sender, const std::string& l_receiver, const std::string& l_content);
    DbResult* getMessagesForUser(const std::string& l_username);
    DbResult* getChatMessages(const std::string& l_firstUser, const std::string& l_secondUser);

    std::string hashPassword(const std::string& l_password);

    bool isRunning() const;
    unsigned int getClientCount() const;
    std::string getClientList() const;
    void setUp();

    std::recursive_mutex& getMutex();
private:
    std::unique_ptr<sf::UdpSocket> m_udpIncoming;
    std::unique_ptr<sf::UdpSocket> m_udpOutgoing;
    std::unique_ptr<sf::TcpListener> m_tcpListener;
    std::unique_ptr<SQLite::Database> m_db;

    sf::SocketSelector m_selector;
    std::unordered_map<ClientID, ClientInfo> m_clientMap;
    std::unordered_map<ClientID, std::shared_ptr<sf::TcpSocket>> m_tcpClients;

    std::thread m_tcpThread;
    std::thread m_udpThread;
    std::recursive_mutex m_clientMutex;
    std::atomic_bool m_running;
    unsigned int m_lastID;

    PacketHandler m_packetHandler;
    TimeoutHandler m_timeoutHandler;
    sf::Time m_serverTime;

    size_t m_totalSent;
    size_t m_totalReceived;
};