#include "World.h"

World::World() : m_server(&World::handlePacket, this), m_running(false), m_entityManager(nullptr),
    m_gameMap(&m_entityManager), m_tick(0), m_tps(0) 
{
    if (!m_server.start()) return;
    m_running = true;

    m_systemManager.setEntityManager(&m_entityManager);
    m_entityManager.setSystemManager(&m_systemManager);

    m_gameMap.loadMap("media/Maps/map1.map");

    m_systemManager.getSystem<S_Movement>(System::Movement)->setMap(&m_gameMap);
    m_systemManager.getSystem<S_Collision>(System::Collision)->setMap(&m_gameMap);
    m_systemManager.getSystem<S_Network>(System::Network)->registerServer(&m_server);

    m_server.bindTimeoutHandler(&World::clientLeave, this);
    m_commandThread = std::thread(&World::commandLine, this);
}

World::~World() { m_entityManager.setSystemManager(nullptr); }

void World::update(const sf::Time& l_time) {
    if (!m_server.isRunning()) { m_running = false; return; }

    m_serverTime += l_time;
    m_snapshotTimer += l_time;
    m_tpsTime += l_time;

    m_server.update(l_time);
    m_server.getMutex().lock();
    m_systemManager.update(l_time.asMilliseconds());
    m_server.getMutex().unlock();

    if (m_snapshotTimer.asMilliseconds() > SNAPSHOT_INTERVAL) {
        sf::Packet packet;
        m_systemManager.getSystem<S_Network>(System::Network)->createSnapshot(packet);
        m_server.broadcast(packet);
        m_snapshotTimer = sf::milliseconds(0);
    }   

    if (m_tpsTime.asMilliseconds() > 1000) {
        m_tps = m_tick;
        m_tick = 0;
        m_tpsTime = sf::milliseconds(0);
    } else {
        ++m_tick;
    }
}

void World::handlePacket(sf::IpAddress& l_ip, const PortNumber& l_port, const PacketID& l_id, sf::Packet& l_packet, Server* l_server) {
    ClientID cid = l_server->getClientID(l_ip, l_port);

    if (cid >= 0) {
        if ((PacketType)l_id == PacketType::Disconnect) {
            clientLeave(cid);
        } else if ((PacketType)l_id == PacketType::Message) {
            // ...
        } else if ((PacketType)l_id == PacketType::Player_Update) {
            m_systemManager.getSystem<S_Network>(System::Network)->updatePlayer(l_packet, cid);
        }
    } else {
        std::lock_guard<std::recursive_mutex> lock(l_server->getMutex());
        if ((PacketType)l_id != PacketType::Connect) return;

        std::string name;
        if (!(l_packet >> name)) return;

        cid = l_server->addClient(NetworkProtocol::UDP, l_ip, l_port);
        if (cid == -1) {
            sf::Packet packet;
            stampPacket(PacketType::Disconnect, packet);
            l_server->send(l_ip, l_port, packet);
            return;
        }

        int eid = m_entityManager.addEntity("Player");
        if (eid == -1) return;

        m_systemManager.getSystem<S_Network>(System::Network)->registerClientID(eid, cid);
        C_Position* pos = m_entityManager.getComponent<C_Position>(eid, Component::Position);
        if (!pos) return;
        pos->setPosition(64.f, 64.f);

        m_entityManager.getComponent<C_Name>(eid, Component::Name)->setName(name);
        
        sf::Packet packet;
        stampPacket(PacketType::Connect, packet);
        packet << eid;
        packet << pos->getPosition().x << pos->getPosition().y;

        if (!m_server.send(cid, packet)) {
            std::cerr << "Failed to send connect packet to the client!" << std::endl;
            return;
        }
    }
}

void World::clientLeave(const ClientID& l_client) {
    std::lock_guard<std::recursive_mutex> lock(m_server.getMutex());
    S_Network* network = m_systemManager.getSystem<S_Network>(System::Network);
    m_entityManager.removeEntity(network->getEntityId(l_client));
}

bool World::isRunning() const { return m_running; }

void World::commandLine() {
    std::string str;
    while (m_server.isRunning()) {
        std::string str;
        std::getline(std::cin, str);
        if (str == "") continue;

        if (str == "terminate") {
            m_server.stop();
            m_running = false;
            break;
        } else if (str == "disconnectall") {
            std::cout << "Disconnecting all clients..." << std::endl;
            m_server.disconnectAll();
            std::lock_guard<std::recursive_mutex> lock(m_server.getMutex());
            m_entityManager.purge();
        } else if (str.find("tps") != std::string::npos) {
            std::cout << "TPS: " << m_tps << std::endl;
        } else if (str.find("health") != std::string::npos) {
            std::stringstream ss(str);

            std::string command;
            std::string eid;
            std::string health;

            if (!(ss >> command)) continue;
            if (!(ss >> eid)) continue;
            if (!(ss >> health)) continue;

            EntityId id = atoi(eid.c_str());
            Health healthValue = atoi(health.c_str());

            C_Health* h = m_entityManager.getComponent<C_Health>(id, Component::Health);
            if (!h) continue;
            h->setHealth(healthValue);
        } else if (str == "clients") {
            std::cout << m_server.getClientCount() << " clients online: " << std::endl;
            std::cout << m_server.getClientList() << std::endl;
        } else if (str == "entities") {
            std::cout << "Current entity count: " << m_entityManager.getEntityCount() << std::endl;
        }
    }
}