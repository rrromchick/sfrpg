#include "Server.h"

Server::Server(void(*l_handler)(sf::IpAddress&, const PortNumber&, const PacketID&, sf::Packet&, Server*)) 
    : m_running(false)
{
    m_packetHandler = std::bind(l_handler, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
    
    m_tcpListener = std::make_unique<sf::TcpListener>();
    m_udpIncoming = std::make_unique<sf::UdpSocket>();
    m_udpOutgoing = std::make_unique<sf::UdpSocket>();
}

Server::~Server() { stop(); }

bool Server::start() {
    if (m_running) return false;
    if (m_udpIncoming->bind((unsigned short)Network::ServerPort) != sf::Socket::Done) return false;
    
    m_udpOutgoing->bind(sf::Socket::AnyPort);
    if (m_tcpListener->listen((unsigned short)Network::ServerPort) != sf::Socket::Done) return false;
    m_selector.add(*m_tcpListener);
    setUp();
    m_running = true;

    m_tcpThread = std::thread(&Server::tcpListen, this);
    m_udpThread = std::thread(&Server::udpListen, this);
    
    std::cout << "UDP incoming listening on " << m_udpIncoming->getLocalPort() << ", outgoing listening on " << m_udpOutgoing->getLocalPort() << std::endl;
    return true;
}

bool Server::stop() {
    if (!m_running) return false;
    
    disconnectAll();
    m_udpIncoming->unbind();
    m_udpOutgoing->unbind();
    m_tcpListener->close();
    m_selector.clear();
    
    m_running = false;
    
    if (m_udpThread.joinable()) {
        m_udpThread.join();
    }
    if (m_tcpThread.joinable()) {
        m_tcpThread.join();
    }

    return true;
}

void Server::disconnectAll() {
    sf::Packet packet;
    stampPacket(PacketType::Disconnect, packet);
    broadcast(packet);
    
    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    m_clientMap.clear();
    m_tcpClients.clear();
}

bool Server::send(const ClientID& l_client, sf::Packet& l_packet) {
    if (!m_running) return false;

    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    auto itr = m_clientMap.find(l_client);
    if (itr == m_clientMap.end()) return false;

    if (itr->second.m_protocol == NetworkProtocol::TCP) {
        auto tcp = m_tcpClients.find(itr->first);
        if (tcp == m_tcpClients.end()) return false;
        if (tcp->second->send(l_packet) != sf::Socket::Done) {
            std::cerr << "Failed sending a packet to tcp client." << std::endl;
            return false;
        }
    } else {
        if (m_udpOutgoing->send(l_packet, itr->second.m_clientIP, itr->second.m_clientPORT) != sf::Socket::Done) {
            std::cout << "Failed sending a packet to udp client." << std::endl;
            return false;
        }
    }

    m_totalSent += l_packet.getDataSize();
    return true;
}

bool Server::send(sf::IpAddress& l_ip, const PortNumber& l_port, sf::Packet& l_packet) {
    if (!m_running) return false;

    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    for (auto itr = m_clientMap.begin(); itr != m_clientMap.end(); ++itr) {
        if (itr->second.m_clientIP != l_ip || itr->second.m_clientPORT != l_port) continue;
        if (itr->second.m_protocol == NetworkProtocol::TCP) {
            auto tcp = m_tcpClients.find(itr->first);
            if (tcp->second->send(l_packet) != sf::Socket::Done) {
                std::cout << "Failed sending a packet to tcp client." << std::endl;
                return false;
            }
        } else {
            if (m_udpOutgoing->send(l_packet, itr->second.m_clientIP, itr->second.m_clientPORT) != sf::Socket::Done) {
                std::cout << "Failed sending a packet to udp client." << std::endl;
                return false;
            }
        }

        m_totalSent += l_packet.getDataSize();
        return true;
    }

    return false;
}

void Server::broadcast(sf::Packet& l_packet, const ClientID& l_ignore) {
    if (!m_running) return;

    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    
    for (auto itr = m_clientMap.begin(); itr != m_clientMap.end(); ++itr) {
        if (itr->first == l_ignore) continue;
        if (itr->second.m_protocol == NetworkProtocol::TCP) {
            auto tcp = m_tcpClients.find(itr->first);
            if (tcp == m_tcpClients.end()) continue;
            tcp->second->setBlocking(true);
            sf::Socket::Status status = tcp->second->send(l_packet);
            if (status != sf::Socket::Done) {
                std::cerr << "Failed to send a packet to tcp client " << itr->second.m_clientIP << ":"
                    << itr->second.m_clientPORT << ". Status code: " << status << std::endl;
                continue;
            }
        } else {
            if (m_udpOutgoing->send(l_packet, itr->second.m_clientIP, itr->second.m_clientPORT) != sf::Socket::Done) {
                std::cerr << "Failed to send a packet to udp client " << itr->second.m_clientIP << ":"
                    << itr->second.m_clientPORT << std::endl;
                continue;
            }
        }

        m_totalSent += l_packet.getDataSize();
    }
}   

void Server::tcpListen() {
    if (!m_running) return;
    m_tcpListener->setBlocking(false);

    sf::Packet packet;
    std::cout << "Beginning to listen (TCP)..." << std::endl;
    while (m_running) {
        if (m_selector.wait(sf::milliseconds(100))) {
            if (m_selector.isReady(*m_tcpListener)) {
                auto newSocket = std::make_shared<sf::TcpSocket>();
                if (m_tcpListener->accept(*newSocket) == sf::Socket::Done) {
                    newSocket->setBlocking(true);
                    m_selector.add(*newSocket);

                    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
                    ClientID cid = addClient(NetworkProtocol::TCP, newSocket->getRemoteAddress(), newSocket->getRemotePort());
                    m_tcpClients.emplace(cid, std::move(newSocket));
                } else {
                    std::cerr << "Error accepting new TCP connection." << std::endl;
                }
            } else {
                std::vector<ClientID> clientsToRemove;
                std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
                for (auto itr = m_tcpClients.begin(); itr != m_tcpClients.end(); ++itr) {
                    if (m_selector.isReady(*itr->second)) {
                        auto itr2 = m_clientMap.find(itr->first);
                        if (itr2 == m_clientMap.end()) continue;

                        sf::Socket::Status status = itr->second->receive(packet);
                        if (status == sf::Socket::Done) {
                            m_totalReceived += packet.getDataSize();

                            PacketID id;
                            if (!(packet >> id)) {
                                std::cerr << "Invalid packet received: unable to extract id." << std::endl;
                                continue;
                            }
                            
                            PacketType pType = (PacketType)id;
                            if (pType < PacketType::Disconnect || pType >= PacketType::OutOfBounds) {
                                std::cerr << "Invalid packet received: unknown type." << std::endl;
                                continue;
                            }

                            if (pType == PacketType::Heartbeat) {
                                if (!itr2->second.m_heartbeatWaiting) {
                                    std::cerr << "Heartbeat packet from unknown client received!" << std::endl;
                                    continue;
                                }
                                itr2->second.m_ping = m_serverTime.asMilliseconds() - itr2->second.m_lastHeartbeat.asMilliseconds();
                                itr2->second.m_heartbeatWaiting = false;
                                itr2->second.m_lastHeartbeat = m_serverTime;
                                itr2->second.m_heartbeatRetry = 0;
                            } else if (m_packetHandler) {
                                m_packetHandler(itr2->second.m_clientIP, itr2->second.m_clientPORT, id, packet, this);
                            }
                        } else if (status == sf::Socket::Disconnected) {
                            removeClient(itr->first);
                            clientsToRemove.emplace_back(itr->first);
                        }
                    }
                }

                for (const auto& id : clientsToRemove) {
                    m_tcpClients.erase(m_tcpClients.find(id));
                }
            }
        }
    }

    std::cout << "Listening (TCP) stopped." << std::endl;
}

void Server::udpListen() {
    if (!m_running) return;

    sf::IpAddress recvIP;
    PortNumber recvPORT;
    sf::Packet packet;

    std::cout << "Beginning to listen (UDP)..." << std::endl;

    while (m_running) {
        packet.clear();

        sf::Socket::Status status = m_udpIncoming->receive(packet, recvIP, recvPORT);

        if (status != sf::Socket::Done) {
            if (m_running) {
                std::cout << "Failed receiving a packet from udp client " << recvIP << ":" << recvPORT  
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
            std::cerr << "Invalid packet received: unable to extract id." << std::endl;
            continue;
        }
        PacketType pType = (PacketType)id;
        if (pType < PacketType::Disconnect || pType >= PacketType::OutOfBounds) {
            std::cerr << "Invalid packet received: unknown type." << std::endl;
            continue;
        }

        if (pType == PacketType::Heartbeat) {
            bool clientFound = false;
            std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
            for (auto itr = m_clientMap.begin(); itr != m_clientMap.end(); ++itr) {
                if (itr->second.m_clientIP != recvIP || itr->second.m_clientPORT != recvPORT) continue;
                clientFound = true;
                if (!itr->second.m_heartbeatWaiting) {
                    std::cout << "Invalid heartbeat packet received!" << std::endl;
                    break;
                }

                itr->second.m_ping = m_serverTime.asMilliseconds() - itr->second.m_lastHeartbeat.asMilliseconds();
                itr->second.m_heartbeatWaiting = false;
                itr->second.m_lastHeartbeat = m_serverTime;
                itr->second.m_heartbeatRetry = 0;
                break;
            }
            if (!clientFound) { std::cout << "Heartbeat packet from unknown client received!" << std::endl; }
        } else if (m_packetHandler) {
            m_packetHandler(recvIP, recvPORT, id, packet, this);
        }
    }

    std::cout << "Listening (UDP) stopped." << std::endl;
}

void Server::update(const sf::Time& l_time) {
    if (!m_running) return;
    m_serverTime += l_time;
    if (m_serverTime.asMilliseconds() < 0) {
        m_serverTime -= sf::milliseconds((sf::Int32)Network::HighestTimestamp);
        std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
        for (auto &itr : m_clientMap) {
            itr.second.m_lastHeartbeat = sf::milliseconds(std::abs(itr.second.m_lastHeartbeat.asMilliseconds()
                - (sf::Int32)Network::HighestTimestamp));
        }
    }

    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    for (auto itr = m_clientMap.begin(); itr != m_clientMap.end(); ) {
        sf::Int32 elapsed = m_serverTime.asMilliseconds() - itr->second.m_lastHeartbeat.asMilliseconds();
        if (elapsed >= HEARTBEAT_INTERVAL) {
            if (elapsed >= (sf::Int32)Network::ClientTimeout || itr->second.m_heartbeatRetry > HEARTBEAT_RETRIES) {
                if (m_timeoutHandler) m_timeoutHandler(itr->first);
                itr = m_clientMap.erase(itr);
                continue;
            }

            if (!itr->second.m_heartbeatWaiting && (elapsed >= HEARTBEAT_INTERVAL * (itr->second.m_heartbeatRetry + 1))) {
                // Heartbeat
                if (itr->second.m_heartbeatRetry >= 3) {
                    std::cout << "Re-try(" << itr->second.m_heartbeatRetry << ") for client " << itr->first << std::endl;
                }

                sf::Packet heartbeat;
                stampPacket(PacketType::Heartbeat, heartbeat);
                heartbeat << m_serverTime.asMilliseconds();
                send(itr->first, heartbeat);

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

void Server::bindTimeoutHandler(void(*l_handler)(const ClientID&)) {
    m_timeoutHandler = std::bind(l_handler, std::placeholders::_1);
}

ClientID Server::addClient(const NetworkProtocol& l_protocol, const sf::IpAddress& l_ip, const PortNumber& l_port) {
    if (!m_running) return (ClientID)Network::NullID;

    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    for (auto itr = m_clientMap.begin(); itr != m_clientMap.end(); ++itr) {
        if (itr->second.m_clientIP == l_ip && itr->second.m_clientPORT == l_port) { 
            return (ClientID)Network::NullID; 
        }
    }

    ClientID cid = m_lastID;
    ClientInfo info(l_protocol, l_ip, l_port, m_serverTime);
    m_clientMap.emplace(cid, info);
    ++m_lastID;
    return cid;
}

ClientID Server::getClientID(const sf::IpAddress& l_ip, const PortNumber& l_port) {
    if (!m_running) return (ClientID)Network::NullID;

    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    for (auto itr = m_clientMap.begin(); itr != m_clientMap.end(); ++itr) {
        if (itr->second.m_clientIP != l_ip || itr->second.m_clientPORT != l_port) continue;
        return itr->first;
    }

    return (ClientID)Network::NullID;
}

bool Server::getClientInfo(const ClientID& l_client, ClientInfo& l_info) {
    if (!m_running) return false;

    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    for (auto itr = m_clientMap.begin(); itr != m_clientMap.end(); ++itr) {
        if (itr->first == l_client) {
            l_info = itr->second;
            return true;
        }
    }

    return false;
}

bool Server::hasClient(const ClientID& l_client) {
    return (m_clientMap.find(l_client) != m_clientMap.end());
}

bool Server::hasClient(const sf::IpAddress& l_ip, const PortNumber& l_port) {
    return (getClientID(l_ip, l_port) >= 0);
}

bool Server::removeClient(const ClientID& l_client) {
    if (!m_running) return false;

    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    auto itr = m_clientMap.find(l_client);
    if (itr == m_clientMap.end()) return false;

    sf::Packet packet;
    stampPacket(PacketType::Disconnect, packet);
    send(l_client, packet);
    m_clientMap.erase(itr);
    
    return true;
}

bool Server::removeClient(const sf::IpAddress& l_ip, const PortNumber& l_port) {
    if (!m_running) return false;

    std::lock_guard<std::recursive_mutex> lock(m_clientMutex);
    for (auto itr = m_clientMap.begin(); itr != m_clientMap.end(); ++itr) {
        if (itr->second.m_clientIP != l_ip || itr->second.m_clientPORT != l_port) continue;
        sf::Packet packet;
        stampPacket(PacketType::Disconnect, packet);
        send(itr->first, packet);
        m_clientMap.erase(itr);
        return true;
    }

    return false;
}

bool Server::isRunning() const { return m_running; }

unsigned int Server::getClientCount() const { return (unsigned int)m_clientMap.size(); }

std::recursive_mutex& Server::getMutex() { return m_clientMutex; }

void Server::setUp() {
    m_running = false;
    m_lastID = 0;
    m_totalSent = 0;
    m_totalReceived = 0;
}

std::string Server::getClientList() const {
    std::string delimeter = "----------------------";
    std::string list = delimeter;
    
    list += '\n';
    list += "ID";
    list += '\t';
    list += "IP:PORT";
    list += '\t'; list += '\t';
    list += "Ping";
    list += '\n';
    
    for (auto itr = m_clientMap.begin(); itr != m_clientMap.end(); ++itr) {
        list += std::to_string(itr->first);
        list += '\t';
        list += itr->second.m_clientIP.toString() + ":" + std::to_string(itr->second.m_clientPORT);
        list += '\t';
        list += std::to_string(itr->second.m_ping);
        list += '\n';
    }

    list += delimeter;
    list += '\n';
    list += "Total sent: " + std::to_string(m_totalSent / 1000) + " kB. Total received: " 
        + std::to_string(m_totalReceived / 1000) + " kB.";
        
    return list;
}