#include "S_Network.h"
#include "System_Manager.h"

S_Network::S_Network(SystemManager* l_sysMgr) : S_Base(System::Network, l_sysMgr) {
    Bitmask req;

    req.turnOnBit((unsigned int)Component::Client);
    m_requiredComponents.emplace_back(req);

    MessageHandler* messageHandler = m_systemManager->getMessageHandler();
    messageHandler->subscribe(EntityMessage::Hurt, this);
    messageHandler->subscribe(EntityMessage::Respawn, this);
    messageHandler->subscribe(EntityMessage::Removed_Entity, this);
}

S_Network::~S_Network() {}

void S_Network::update(float l_dT) {
    for (auto& entity : m_entities) {
        auto& player = m_input[entity];
        
        if (player.m_movedX || player.m_movedY) {
            if (player.m_movedX) {
                Message msg((MessageType)EntityMessage::Move);
                msg.m_receiver = entity;
                if (player.m_movedX > 0) msg.m_int = (int)Direction::Right;
                else msg.m_int = (int)Direction::Left;
                m_systemManager->getMessageHandler()->dispatch(msg);
            }

            if (player.m_movedY) {
                Message msg((MessageType)EntityMessage::Move);
                msg.m_receiver = entity;
                if (player.m_movedY > 0) msg.m_int = (int)Direction::Down;
                else msg.m_int = (int)Direction::Up;
                m_systemManager->getMessageHandler()->dispatch(msg);
            }
        }

        if (player.m_attacking) {
            Message msg((MessageType)EntityMessage::Attack);
            msg.m_receiver = entity;
            m_systemManager->getMessageHandler()->dispatch(msg);
        }
    }
}

void S_Network::handleEvent(const EntityId& l_entity, const EntityEvent& l_event) {}

void S_Network::notify(const Message& l_message) {
    if (!hasEntity(l_message.m_receiver)) return;
    EntityMessage m = (EntityMessage)l_message.m_type;

    if (m == EntityMessage::Removed_Entity) { m_input.erase(l_message.m_receiver); return; }

    if (m == EntityMessage::Hurt) {
        sf::Packet packet;
        stampPacket(PacketType::Hurt, packet);
        packet << l_message.m_receiver;
        m_server->broadcast(packet);
        return;
    }

    if (m == EntityMessage::Respawn) {
        C_Position* pos = m_systemManager->getEntityManager()->getComponent<C_Position>(l_message.m_receiver, Component::Position);
        if (!pos) return;
        pos->setPosition(64.f, 64.f);
        pos->setElevation(1);
    }
}

void S_Network::registerServer(Server* l_server) { m_server = l_server; }

bool S_Network::registerClientID(const EntityId& l_entity, const ClientID& l_client) {
    if (!hasEntity(l_entity)) return false;

    m_systemManager->getEntityManager()->getComponent<C_Client>(l_entity, Component::Client)->setClientID(l_client);

    return true;
}

EntityId S_Network::getEntityId(const ClientID& l_client) {
    EntityManager* entityManager = m_systemManager->getEntityManager();
    auto itr = std::find_if(m_entities.begin(), m_entities.end(), [&entityManager, &l_client](const EntityId& l_entity) {
        return entityManager->getComponent<C_Client>(l_entity, Component::Client)->getClientID() == l_client;
    });
    return (itr != m_entities.end() ? *itr : (EntityId)Network::NullID);
}

ClientID S_Network::getClientID(const EntityId& l_entity) {
    if (!hasEntity(l_entity)) return (ClientID)Network::NullID;
    return m_systemManager->getEntityManager()->getComponent<C_Client>(l_entity, Component::Client)->getClientID();
}

void S_Network::createSnapshot(sf::Packet& l_packet) {
    std::lock_guard<std::recursive_mutex> lock(m_server->getMutex());

    ServerEntityManager* entityManager = (ServerEntityManager*)m_systemManager->getEntityManager();
    stampPacket(PacketType::Snapshot, l_packet);
    l_packet << (sf::Int32)entityManager->getEntityCount();
    if (entityManager->getEntityCount() > 0) {
        entityManager->dumpEntityInfo(l_packet);
    }
}

void S_Network::updatePlayer(sf::Packet& l_packet, const ClientID& l_client) {
    std::lock_guard<std::recursive_mutex> lock(m_server->getMutex());

    EntityId eid = getEntityId(l_client);
    if (eid == -1) return;
    if (!hasEntity(eid)) return;

    m_input[eid].m_attacking = false;
    sf::Int8 entity_message;
    while (l_packet >> entity_message) {
        EntityMessage m = (EntityMessage)entity_message;
        switch (m) {
            case EntityMessage::Move: {
                sf::Int32 x = 0, y = 0;
                l_packet >> x >> y;
                m_input[eid].m_movedX = x;
                m_input[eid].m_movedY = y;
                break;
            }
            case EntityMessage::Attack: {
                sf::Int8 attackingState;
                l_packet >> attackingState;
                if (attackingState) m_input[eid].m_attacking = true;
                break;
            }
        }

        sf::Int8 delim = 0;
        if (!(l_packet >> delim) || (delim != (sf::Int8)Network::PlayerUpdateDelim)) {
            std::cerr << "Faulty update!" << std::endl;
            break;
        }
    }
}