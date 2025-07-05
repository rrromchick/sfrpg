#pragma once

#include "Entity_Manager.h"
#include "C_Position.h"
#include "C_Movable.h"
#include "C_State.h"
#include "C_Attacker.h"
#include "C_Controller.h"
#include "C_Collidable.h"
#include "C_Health.h"
#include "C_Name.h"
#include "C_Client.h"
#include "EntitySnapshot.h"

class ServerEntityManager : public EntityManager {
public:
    ServerEntityManager(SystemManager* l_systemMgr);
    ~ServerEntityManager();

    void dumpEntityInfo(sf::Packet& l_packet);
private:

};