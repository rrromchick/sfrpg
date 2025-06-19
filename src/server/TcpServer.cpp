#include "TcpServer.h"

TcpServer::TcpServer(void(*l_handler)(sf::TcpSocket*, const PacketID&, sf::Packet&, TcpServer*)) 
    : m_listenThread(&TcpServer::Listen, this), m_running(false)
{
    m_packetHandler = std::bind(l_handler, std::placeholders::_1, 
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
}

TcpServer::~TcpServer() { Stop(); }

bool TcpServer::Start() {
    if (m_running) return false;
    if (m_listener.listen((unsigned short)Network::ServerPort) != sf::Socket::Done) {
        std::cout << "Error: could not bind listener to port." << std::endl;
        return false;
    }
    m_selector.add(m_listener);
    std::cout << "Started listening on port " << (unsigned short)Network::ServerPort << std::endl;
    m_running = true;
    m_listenThread.launch();
    return true;
}

void TcpServer::DisconnectAll() {
    if (!m_running) return;
    sf::Packet packet;
    StampPacket(PacketType::Disconnect, packet);
    Broadcast(packet);
    sf::Lock lock(m_mutex);
    m_clients.clear();
}

bool TcpServer::Stop() {
    if (!m_running) return false;
    DisconnectAll();
    m_running = false;
    m_selector.remove(m_listener);
    return true;
}

bool TcpServer::Send(const ClientID& l_client, sf::Packet& l_packet) {
    if (!m_running) return false;
    sf::Lock lock(m_mutex);
    auto itr = m_clients.find(l_client);
    if (itr == m_clients.end()) return false;
    if (itr->second.m_socket->send(l_packet) != sf::Socket::Done) {
        std::cout << "Failed sending a packet to client " << l_client << std::endl;
        return false;
    }
    m_totalSent += l_packet.getDataSize();
    return true;
}

bool TcpServer::Send(sf::IpAddress& l_ip, const PortNumber& l_port, sf::Packet& l_packet) {
    if (!m_running) return false;
    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        if (itr->second.m_clientIP != l_ip || itr->second.m_clientPORT != l_port) continue;
        if (itr->second.m_socket->send(l_packet) != sf::Socket::Done) {
            std::cout << "Failed sending a packet to client " << itr->first << std::endl;
            return false;
        }
        m_totalSent += l_packet.getDataSize();
        return true;
    }   
    return false;
}

void TcpServer::Broadcast(sf::Packet& l_packet, const ClientID& l_ignore) {
    if (!m_running) return;
    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        if (itr->first == l_ignore) continue;
        if (itr->second.m_socket->send(l_packet) != sf::Socket::Done) {
            std::cout << "Failed sending a packet to client " << itr->first << std::endl;
            continue;
        }
        m_totalSent += l_packet.getDataSize();
    }
}

void TcpServer::Listen() {
    std::cout << "Beginning to listen..." << std::endl;
    while (m_running) {
        if (m_selector.wait()) {
            sf::Lock lock(m_mutex);

            for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
                if (m_selector.isReady(*itr->second.m_socket)) {
                    sf::Packet packet;
                    sf::Socket::Status status = itr->second.m_socket->receive(packet);

                    if (status != sf::Socket::Done) {
                        if (m_running) {
                            std::cout << "Failed receiving a packet from client " << itr->second.m_clientIP << ":" << itr->second.m_clientPORT  
                                << ". Status code: " << status << std::endl;
                            continue;
                        } else {
                            std::cout << "Socket unbound." << std::endl;
                            break;
                        }
                    }

                    m_totalReceived += packet.getDataSize();

                    PacketID id;
                    if (!(packet >> id)) {
                        std::cout << "Invalid packet received: unable to extract id." << std::endl;
                        continue;
                    }
                    PacketType pType = (PacketType)id;
                    if (pType < PacketType::Disconnect || pType >= PacketType::OutOfBounds) {
                        std::cout << "Invalid packet received: unknown type." << std::endl;
                        continue;
                    }

                    if (pType == PacketType::Heartbeat) {
                        if (!itr->second.m_heartbeatWaiting) {
                            std::cout << "Invalid heartbeat packet received!" << std::endl;
                            continue;
                        }
                        itr->second.m_ping = m_serverTime.asMilliseconds() - itr->second.m_lastHeartbeat.asMilliseconds();
                        itr->second.m_lastHeartbeat = m_serverTime;
                        itr->second.m_heartbeatWaiting = false;
                        itr->second.m_heartbeatRetry = 0;
                    } else if (m_packetHandler) {
                        m_packetHandler(itr->second.m_socket, id, packet, this);
                    }
                }
            }
        }
    }
}

void TcpServer::Update(const sf::Time& l_time) {
    m_serverTime += l_time;
    if (m_serverTime.asMilliseconds() < 0) {
        m_serverTime -= sf::milliseconds((sf::Int32)Network::HighestTimestamp);
        sf::Lock lock(m_mutex);
        for (auto &itr : m_clients) {
            itr.second.m_lastHeartbeat = sf::milliseconds(std::abs(itr.second.m_lastHeartbeat.asMilliseconds() 
                - (sf::Int32)Network::HighestTimestamp));
        }
    }

    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ) {
        sf::Int32 elapsed = m_serverTime.asMilliseconds() - itr->second.m_lastHeartbeat.asMilliseconds();
        if (elapsed >= HEARTBEAT_INTERVAL) {
            if (elapsed >= (sf::Int32)Network::ClientTimeout || (itr->second.m_heartbeatRetry > HEARTBEAT_RETRIES)) {
                if (m_timeoutHandler) m_timeoutHandler(itr->first);
                itr = m_clients.erase(itr);
                continue;
            }
            if (!itr->second.m_heartbeatWaiting || (elapsed >= HEARTBEAT_INTERVAL * (itr->second.m_heartbeatRetry + 1))) {
                // Heartbeat
                if (itr->second.m_heartbeatRetry >= 3) {
                    std::cout << "Re-try(" << itr->second.m_heartbeatRetry << ") for client " << itr->first << std::endl;
                }
                sf::Packet packet;
                StampPacket(PacketType::Heartbeat, packet);
                packet << m_serverTime.asMilliseconds();
                Send(itr->second.m_clientIP, itr->second.m_clientPORT, packet);

                if (itr->second.m_heartbeatRetry == 0) {
                    itr->second.m_heartbeatSent = m_serverTime;
                }
                itr->second.m_heartbeatWaiting = true;
                ++itr->second.m_heartbeatRetry;

                m_totalSent += packet.getDataSize();
            }
        }
    }
}

void TcpServer::BindTimeoutHandler(void(*l_handler)(const ClientID&)) {
    m_timeoutHandler = std::bind(l_handler, std::placeholders::_1);
}

ClientID TcpServer::AddClient(sf::TcpSocket* l_socket) {
    if (!m_running) return (ClientID)Network::NullID;
    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        if (itr->second.m_socket == l_socket) return (ClientID)Network::NullID;
    }

    ClientID cid = m_lastID;
    ClientInfo info(l_socket, m_serverTime);
    m_clients.emplace(cid, info);
    ++m_lastID;
    return cid;
}

ClientID TcpServer::GetClientID(sf::TcpSocket* l_socket) {
    if (!m_running) return (ClientID)Network::NullID;
    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        if (itr->second.m_socket == l_socket) {
            return itr->first;
        }
    }
    return (ClientID)Network::NullID;
}

bool TcpServer::HasClient(const ClientID& l_client) {
    return (m_clients.find(l_client) != m_clients.end());
}

bool TcpServer::HasClient(sf::TcpSocket* l_socket) {
    return (GetClientID(l_socket) >= 0);
}

bool TcpServer::RemoveClient(const ClientID& l_client) {
    if (!m_running) return false;
    sf::Lock lock(m_mutex);
    auto itr = m_clients.find(l_client);
    if (itr == m_clients.end()) return false;
    sf::Packet packet;
    StampPacket(PacketType::Disconnect, packet);
    Send(l_client, packet);
    m_clients.erase(itr);
    m_totalSent += packet.getDataSize();
    return true;
}

bool TcpServer::RemoveClient(sf::TcpSocket* l_socket) {
    if (!m_running) return false;
    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        if (itr->second.m_socket == l_socket) {
            sf::Packet packet;
            StampPacket(PacketType::Disconnect, packet);
            l_socket->send(packet);
            m_clients.erase(itr);
            m_totalSent += packet.getDataSize();
            return true;
        }
    }
    return false;
}

bool TcpServer::GetClientInfo(const ClientID& l_client, ClientInfo& l_info) {
    if (!m_running) return false;
    sf::Lock lock(m_mutex);
    auto itr = m_clients.find(l_client);
    if (itr == m_clients.end()) return false;
    l_info = itr->second;
    return true;
}

unsigned int TcpServer::GetClientCount() { return (unsigned int)m_clients.size(); }

bool TcpServer::IsRunning() const { return m_running; }

void TcpServer::Setup() {
    m_lastID = 0;
    m_running = false;
    m_totalSent = 0;
    m_totalReceived = 0;
}

sf::Mutex& TcpServer::GetMutex() { return m_mutex; }

std::string TcpServer::GetClientList() {
    std::string delimeter = "---------------------------";
    std::string list = delimeter;
    list += '\n';
    list += "ID";
    list += '\t';
    list += "IP:PORT";
    list += '\t'; list += '\t';
    list += "Ping";
    list += '\n';
    list += delimeter;
    list += '\n';
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        list += std::to_string(itr->first);
        list += '\t';
        list += itr->second.m_clientIP.toString() + ":" + std::to_string(itr->second.m_clientPORT);
        list += '\t';
        list += std::to_string(itr->second.m_ping);
        list += '\n';
    }
    list += delimeter;
    list += '\n';
    list += "Total sent: " + std::to_string(m_totalSent / 1000) + "kB. Total received: " + std::to_string(m_totalReceived / 1000) + " kB.";
    return list;
}