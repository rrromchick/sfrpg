#include "Client.h"

Client::Client(const NetworkProtocol& l_protocol)
    : m_protocol(l_protocol), m_connected(false)
{
    m_udpSocket = std::make_unique<sf::UdpSocket>();
    m_tcpSocket = std::make_shared<sf::TcpSocket>();
}

Client::~Client() {}

bool Client::connect() {
    if (m_connected) return false;

    if (m_protocol == NetworkProtocol::UDP) {
        m_udpSocket->bind(sf::Socket::AnyPort);
        sf::Packet packet;
        stampPacket(PacketType::Connect, packet);
        if (m_udpSocket->send(packet, m_serverIp, m_serverPort) != sf::Socket::Done) { m_udpSocket->unbind(); return false; }
        
        packet.clear();
        sf::IpAddress recvIP;
        PortNumber recvPORT;
        m_udpSocket->setBlocking(false);

        sf::Clock clock;
        clock.restart();

        std::cout << "Attempting to connect to the server (UDP) " << m_serverIp << ":" << m_serverPort << std::endl;
        while (clock.getElapsedTime().asMilliseconds() < CONNECT_TIMEOUT) {
            packet.clear();

            sf::Socket::Status status = m_udpSocket->receive(packet, recvIP, recvPORT);
            if (status != sf::Socket::Done) continue;

            if (recvIP != m_serverIp) continue;

            PacketID id;
            if (!(packet >> id)) {
                std::cerr << "Invalid packet received: unable to extract id." << std::endl;
                continue;
            }
            PacketType pType = (PacketType)id;
            if (pType != PacketType::Connect) continue;
            
            m_packetHandler(id, packet, this);
            m_connected = true;
            m_udpSocket->setBlocking(true);
            m_clientThread = std::thread(&Client::udpListen, this);
            m_lastHeartbeat = m_serverTime;

            return true;
        } 

        std::cout << "Failed to connect to the server " << m_serverIp << ":" << m_serverPort << std::endl;
        m_udpSocket->setBlocking(true);
        m_udpSocket->unbind();
        return false;
    }

    std::cout << "Attempting to connect to the server (TCP) " << m_serverIp << ":" << m_serverPort << std::endl;
    if (m_tcpSocket->connect(m_serverIp, m_serverPort, sf::milliseconds(CONNECT_TIMEOUT)) != sf::Socket::Done) {
        std::cerr << "Failed to connect to the server (TCP)!" << std::endl;
        return false;
    }
    
    m_clientThread = std::thread(&Client::tcpListen, this);
    m_tcpSocket->setBlocking(true);
    m_connected = true;
    m_lastHeartbeat = m_serverTime;
    return true;
}

bool Client::disconnect() {
    if (!m_connected) return false;

    if (m_protocol == NetworkProtocol::TCP) {
        m_tcpSocket->disconnect();
    } else {
        sf::Packet packet;
        stampPacket(PacketType::Disconnect, packet);
        m_udpSocket->send(packet, m_serverIp, m_serverPort);

        m_udpSocket->unbind();
    }

    m_connected = false;
    
    if (m_clientThread.joinable()) {
        m_clientThread.join();
    }

    return true;
}

void Client::udpListen() {
    sf::IpAddress recvIP;
    PortNumber recvPORT;
    sf::Packet packet;

    std::cout << "Beginning to listen (UDP) ..." << std::endl;
    while (m_connected) {
        packet.clear();

        sf::Socket::Status status = m_udpSocket->receive(packet, recvIP, recvPORT);

        if (status != sf::Socket::Done) {
            if (m_connected) {
                std::cerr << "Failed receiving a packet from " << recvIP << ":" << recvPORT 
                    << ". Status code: " << status << std::endl;
                continue;
            } else {
                std::cout << "Socket unbound." << std::endl;
                break;
            }
        }

        if (recvIP != m_serverIp) {
            std::cout << "Packet from unknown server received!" << std::endl;
            continue;
        }

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
            sf::Packet heartbeat;
            stampPacket(PacketType::Heartbeat, heartbeat);
            send(heartbeat);

            sf::Int32 timestamp;
            packet >> timestamp;
            setTime(sf::milliseconds(timestamp));
            m_lastHeartbeat = m_serverTime;
        } else if (m_packetHandler) {
            m_packetHandler(id, packet, this);
        }
    }

    std::cout << "Listening (UDP) stopped." << std::endl;
}

void Client::tcpListen() {
    sf::Packet packet;

    std::cout << "Beginning to listen (TCP)..." << std::endl;
    while (m_connected) {
        packet.clear();

        sf::Socket::Status status = m_tcpSocket->receive(packet);

        if (status != sf::Socket::Done) {
            if (status == sf::Socket::Disconnected) {
                disconnect();
                break;
            }
            
            if (m_connected) {
                std::cerr << "Failed receiving a packet from server " << m_serverIp << ":" << m_serverPort  
                    << ". Status code: " << status << std::endl;
                continue;
            } else {
                std::cout << "Socket disconnected." << std::endl;
                break;
            }
        }

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
            sf::Packet heartbeat;
            stampPacket(PacketType::Heartbeat, heartbeat);
            send(heartbeat);

            sf::Int32 timestamp;
            packet >> timestamp;
            setTime(sf::milliseconds(timestamp));
            m_lastHeartbeat = m_serverTime;
        } else if (m_packetHandler) {
            m_packetHandler(id, packet, this);
        }
    }

    std::cout << "Listening (TCP) stopped." << std::endl;
}

bool Client::send(sf::Packet& l_packet) {
    if (!m_connected) return false;

    if (m_protocol == NetworkProtocol::TCP) {
        if (m_tcpSocket->send(l_packet) != sf::Socket::Done) {
            std::cerr << "Failed sending a packet to the server." << std::endl;
            return false;
        }
    } else {
        if (m_udpSocket->send(l_packet, m_serverIp, m_serverPort) != sf::Socket::Done) {
            std::cerr << "Failed sending a packet to the server." << std::endl;
            return false;
        } 
    }
    return true;
}

void Client::update(const sf::Time& l_time) {
    if (!m_connected) return;

    m_serverTime += l_time;
    if (m_serverTime.asMilliseconds() < 0) {
        m_serverTime -= sf::milliseconds((sf::Int32)Network::HighestTimestamp);
        m_lastHeartbeat = m_serverTime;
        return;
    }

    if (m_serverTime.asMilliseconds() - m_lastHeartbeat.asMilliseconds() > (sf::Int32)Network::ClientTimeout) {
        std::cout << "Client has timed out. Server time: " << m_serverTime.asMilliseconds() 
            << ", last heartbeat: " << m_lastHeartbeat.asMilliseconds() << std::endl;
        disconnect();
    }
}

void Client::setUp(void(*l_handler)(const PacketID&, sf::Packet&, Client*)) {
    m_packetHandler = std::bind(l_handler, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);
}

void Client::setServerInformation(const sf::IpAddress& l_ip, const PortNumber& l_port) {
    m_serverIp = l_ip;
    m_serverPort = l_port;
}

void Client::unregisterPacketHandler() { m_packetHandler = nullptr; }

const sf::Time& Client::getTime() const { return m_serverTime; }

const sf::Time& Client::getLastHeartbeat() const { return m_lastHeartbeat; }

void Client::setTime(const sf::Time& l_time) { m_serverTime = l_time; }

std::mutex& Client::getMutex() { return m_clientMutex; }

bool Client::isConnected() const { return m_connected; }