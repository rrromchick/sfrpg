#pragma once

#include "S_Base.h"
#include "Server.h"
#include "Server_Entity_Manager.h"
#include "S_State.h"
#include "S_Movement.h"
#include "C_Client.h"

struct PlayerInput {
    int m_movedX;
    int m_movedY;
    bool m_attacking;

    PlayerInput() : m_movedX(0), m_movedY(0), m_attacking(false) {}
};

using PlayerInputContainer = std::unordered_map<EntityId, PlayerInput>;

class S_Network : public S_Base {
public:
    S_Network(SystemManager* l_systemMgr);
    ~S_Network();

    void update(float l_dT);
    void handleEvent(const EntityId& l_entity, const EntityEvent& l_event);
    void notify(const Message& l_message);

    void registerServer(Server* l_server);
    bool registerClientID(const EntityId& l_entity, const ClientID& l_client);

    EntityId getEntityId(const ClientID& l_client);
    ClientID getClientID(const EntityId& l_entity);

    void createSnapshot(sf::Packet& l_packet);
    void updatePlayer(sf::Packet& l_packet, const ClientID& l_entity);
private:
    PlayerInputContainer m_input;
    Server* m_server;
};