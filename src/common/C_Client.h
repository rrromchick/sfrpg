#pragma once

#include "C_Base.h"
#include "NetworkDefinitions.h"

class C_Client : public C_Base {
public:
    C_Client() : C_Base(Component::Client), m_clientID((ClientID)Network::NullID) 
    {}
    
    void readIn(std::stringstream& l_stream) {}

    ClientID getClientID() const { return m_clientID; }
    void setClientID(const ClientID& l_client) { m_clientID = l_client; }
private:
    ClientID m_clientID;
};