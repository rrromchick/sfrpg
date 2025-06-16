#include "UdpServer.h"

UdpServer::UdpServer(void(*l_handler)(sf::IpAddress&, const PortNumber&, const PacketID&, sf::Packet&, UdpServer*)) 
    : m_listenThread(&UdpServer::Listen, this), m_running(false)
{
    m_packetHandler = std::bind(l_handler, std::placeholders::_1, std::placeholders::_2, 
        std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
}

UdpServer::~UdpServer() { Stop(); }

void UdpServer::BindTimeoutHandler(void(*l_handler)(const ClientID&)) {
    m_timeoutHandler = std::bind(l_handler, std::placeholders::_1);
}

bool UdpServer::Start() {
    if (m_running) return false;
    if (m_incoming.bind((unsigned short)Network::ServerPort) != sf::Socket::Done) return false;
    m_outgoing.bind(sf::Socket::AnyPort);
    Setup();
    std::cout << "Incoming socket on " << m_incoming.getLocalPort() << ", outgoing on " << m_outgoing.getLocalPort() << std::endl;
    m_listenThread.launch();
    m_running = true;
    return true;
}

void UdpServer::DisconnectAll() {
    if (!m_running) return;
    sf::Packet packet;
    StampPacket(PacketType::Disconnect, packet);
    Broadcast(packet);
    sf::Lock lock(m_mutex);
    m_clients.clear();
}

bool UdpServer::Stop() {
    if (!m_running) return false;
    DisconnectAll();
    m_incoming.unbind();
    m_running = false;
    return true;
}

void UdpServer::Listen() {
    sf::IpAddress recvIP;
    PortNumber recvPORT;
    sf::Packet packet;

    std::cout << "Beginning to listen..." << std::endl;
    while (m_running) {
        packet.clear();

        sf::Socket::Status status = m_incoming.receive(packet, recvIP, recvPORT);

        if (status != sf::Socket::Done) {
            if (m_running) {
                std::cout << "Failed receiving a packet from " << recvIP << " " << recvPORT 
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
            std::cout << "Invalid packet received: unknown packet type." << std::endl;
            continue;
        }

        if (pType == PacketType::Heartbeat) {
            bool clientFound = false;
            sf::Lock lock(m_mutex);
            for (auto &itr : m_clients) {
                if (itr.second.m_clientIP != recvIP || itr.second.m_clientPORT != recvPORT) continue;
                if (!itr.second.m_heartbeatWaiting) {
                    std::cout << "Invalid heartbeat packet received!" << std::endl;
                    continue;
                }
                itr.second.m_lastHeartbeat = m_serverTime;
                itr.second.m_heartbeatWaiting = false;
                itr.second.m_ping = m_serverTime.asMilliseconds() - itr.second.m_lastHeartbeat.asMilliseconds();
                itr.second.m_heartbeatRetry = 0;
                break;
            }
            if (clientFound) {
                std::cout << "A heartbeat from unknown client received." << std::endl;
            }
        } else if (m_packetHandler) {
            m_packetHandler(recvIP, recvPORT, id, packet, this);
        }
    }

    std::cout << "Listening stopped." << std::endl;
}

void UdpServer::Update(const sf::Time &l_time) {
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
            if (elapsed >= (sf::Int32)Network::ClientTimeout || itr->second.m_heartbeatRetry > HEARTBEAT_RETRIES) {
                ClientID cid = GetClientID(itr->second.m_clientIP, itr->second.m_clientPORT);
                if (m_timeoutHandler) m_timeoutHandler(cid);
                itr = m_clients.erase(itr);
                continue;
            }
            if (!itr->second.m_heartbeatWaiting && (elapsed >= (HEARTBEAT_INTERVAL * (itr->second.m_heartbeatRetry + 1)))) {
                // Heartbeat
                if (itr->second.m_heartbeatRetry >= 3) {
                    std::cout << "Re-try(" << itr->second.m_heartbeatRetry << ") for client: " << itr->first << std::endl;
                }
                sf::Packet heartbeat;
                StampPacket(PacketType::Heartbeat, heartbeat);
                heartbeat << m_serverTime.asMilliseconds();
                
                Send(itr->first, heartbeat);

                if (itr->second.m_heartbeatRetry == 0) {
                    itr->second.m_heartbeatSent = m_serverTime;
                }

                itr->second.m_heartbeatWaiting = true;
                ++itr->second.m_heartbeatRetry;

                m_totalSent += heartbeat.getDataSize();
            }
        }

        ++itr;
    }
}

bool UdpServer::Send(sf::IpAddress &l_ip, const PortNumber &l_port, sf::Packet &l_packet) {
    if (!m_running) return false;
    sf::Lock lock(m_mutex);
    if (m_outgoing.send(l_packet, l_ip, l_port) != sf::Socket::Done) {
        std::cout << "Failed sending a packet to " << l_ip << ":" << l_port << std::endl;
        return false;
    }
    m_totalSent += l_packet.getDataSize();
    return true;
}

bool UdpServer::Send(const ClientID &l_client, sf::Packet &l_packet) {
    if (!m_running) return false;
    sf::Lock lock(m_mutex);
    auto itr = m_clients.find(l_client);
    if (itr == m_clients.end()) return false;
    if (m_outgoing.send(l_packet, itr->second.m_clientIP, itr->second.m_clientPORT) != sf::Socket::Done) {
        std::cout << "Failed sending a packet to " << itr->second.m_clientIP << ":" << itr->second.m_clientPORT << std::endl;
        return false;
    }
    m_totalSent += l_packet.getDataSize();
    return true;
}

void UdpServer::Broadcast(sf::Packet &l_packet, const ClientID &l_ignore) {
    if (!m_running) return;
    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        if (itr->first != l_ignore) {
            if (m_outgoing.send(l_packet, itr->second.m_clientIP, itr->second.m_clientPORT) != sf::Socket::Done) {
                std::cout << "Failed sending a packet to " << itr->second.m_clientIP << ":" << itr->second.m_clientPORT << std::endl;
                continue;
            }
            m_totalSent += l_packet.getDataSize();
        }
    }
}

bool UdpServer::AddClient(const sf::IpAddress &l_ip, const PortNumber &l_port) {
    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        if (itr->second.m_clientIP == l_ip && itr->second.m_clientPORT == l_port) return false;
    }
    ClientID cid = m_lastID;
    ClientInfo info(l_ip, l_port, m_serverTime);
    if (!m_clients.emplace(cid, info).second) return false;
    ++m_lastID;
    return true;
}

ClientID UdpServer::GetClientID(const sf::IpAddress &l_ip, const PortNumber &l_port) {
    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        if (itr->second.m_clientIP == l_ip && itr->second.m_clientPORT == l_port) {
            return itr->first;
        }
    }
    return (ClientID)Network::NullID;
}

bool UdpServer::HasClient(const sf::IpAddress &l_ip, const PortNumber &l_port) {
    return (GetClientID(l_ip, l_port) >= 0);
}

bool UdpServer::HasClient(const ClientID &l_client) {
    return (m_clients.find(l_client) != m_clients.end());
}

bool UdpServer::RemoveClient(const sf::IpAddress &l_ip, const PortNumber &l_port) {
    sf::Lock lock(m_mutex);
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        if (itr->second.m_clientIP == l_ip && itr->second.m_clientPORT == l_port) {
            sf::Packet packet;
            StampPacket(PacketType::Disconnect, packet);
            Send(itr->first, packet);
            m_clients.erase(itr);
            return true;
        }
    }
    return false;
}

bool UdpServer::RemoveClient(const ClientID &l_client) {
    sf::Lock lock(m_mutex);
    auto itr = m_clients.find(l_client);
    if (itr == m_clients.end()) return false;
    sf::Packet packet;
    StampPacket(PacketType::Disconnect, packet);
    Send(l_client, packet);
    m_clients.erase(itr);
    return true;
}

bool UdpServer::GetClientInfo(const ClientID &l_client, ClientInfo &l_info) {
    sf::Lock lock(m_mutex);
    for (auto &itr : m_clients) {
        if (itr.first == l_client) {
            l_info = itr.second;
            return true;
        }
    }
    return false;
}

bool UdpServer::IsRunning() const { return m_running; }

unsigned int UdpServer::GetClientCount() { return (unsigned int)m_clients.size(); }

sf::Mutex& UdpServer::GetMutex() { return m_mutex; }

std::string UdpServer::GetClientList() {
    std::string delimeter = "-----------------------";
    std::string list = delimeter;
    list += '\n';
    list += "ID";
    list += '\t';
    list += "IP:PORT"; 
    list += '\t'; list += '\t';
    list += "Ping";
    list += '\t';
    list += '\n';
    list += delimeter;
    list += '\n';
    for (auto itr = m_clients.begin(); itr != m_clients.end(); ++itr) {
        list += itr->first;
        list += '\t';
        list += itr->second.m_clientIP.toString() + ":" + std::to_string(itr->second.m_clientPORT);
        list += '\t';
        list += std::to_string(itr->second.m_ping);
        list += '\n';
    }
    list += delimeter;
    list += "Total data sent: " + std::to_string(m_totalSent / 1000) + "kB. Total data received: " + std::to_string(m_totalReceived / 1000) + "kB.";
    list += '\n';
    return list;
}

void UdpServer::Setup() {
    m_running = false;
    m_lastID = 0;
    m_totalSent = 0;
    m_totalReceived = 0;
}