#pragma once

#include "Server.h"
#include "Server_Entity_Manager.h"
#include "Server_System_Manager.h"
#include "Map.h"
#include "NetSettings.h"

class World {
public:
    World();
    ~World();

    void update(const sf::Time& l_time);
    void handlePacket(sf::IpAddress& l_ip, const PortNumber& l_port, const PacketID& l_id, sf::Packet& l_packet, Server*);
    void clientLeave(const ClientID& l_client);
    void commandLine();
    
    bool isRunning() const;
private:
    sf::Time m_tpsTime;
    sf::Time m_serverTime;
    sf::Time m_snapshotTimer;

    ServerEntityManager m_entityManager;
    ServerSystemManager m_systemManager;
    
    std::thread m_commandThread;
    Server m_server;
    bool m_running;
    
    unsigned int m_tick;
    unsigned int m_tps;
    Map m_gameMap;
};