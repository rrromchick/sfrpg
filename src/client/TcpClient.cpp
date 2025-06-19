#include "TcpClient.h"

TcpClient::TcpClient() : m_listenThread(&TcpClient::Listen, this), m_connected(false) {}

TcpClient::~TcpClient() {}

bool TcpClient::Connect() {
    if (m_connected) return false;
    sf::Socket::Status status = m_socket.connect(m_serverIp, m_serverPort);
    if (status != sf::Socket::Done) { m_socket.disconnect(); return false; }
    
    sf::Packet packet;
    StampPacket(PacketType::Connect, packet);
    packet << m_playerName;
    Send(packet);
    packet.clear();
    m_socket.setBlocking(false);
    sf::Clock clock;
    clock.restart();
    
    std::cout << "Attempting to connect to " << m_serverIp << ":" << m_serverPort << std::endl;
    while (clock.getElapsedTime().asMilliseconds() < CONNECT_TIMEOUT) {
        packet.clear();
        sf::Socket::Status status = m_socket.receive(packet);
        if (status != sf::Socket::Done) {
            if (m_connected) {
                if (m_connected) {
                    std::cout << "Failed to receive a packet from " << m_serverIp << ":" << m_serverPort    
                        << ". Status code: " << status << std::endl;
                    continue;
                } else {
                    std::cout << "Socket unbound." << std::endl;
                    break;
                }
            }
        }

        PacketID id;
        if (!(packet >> id)) continue;
        PacketType pType = (PacketType)id;
        if (pType != PacketType::Connect) continue;
        
        m_packetHandler(id, packet, this);
        m_connected = true;
        m_socket.setBlocking(true);
        m_listenThread.launch();
        return true;
    }
    std::cout << "Failed to connect to " << m_serverIp << ":" << m_serverPort << std::endl;
    m_socket.setBlocking(true);
    m_socket.disconnect();
    return false;
}

void TcpClient::Listen() {
    sf::Packet packet;
    while (m_connected) {
        packet.clear();
        sf::Socket::Status status = m_socket.receive(packet);
        
        if (status != sf::Socket::Done) {
            if (m_connected) {
                std::cout << "Failed receiving a packet from " << m_serverIp << ":" << m_serverPort 
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
            StampPacket(PacketType::Heartbeat, heartbeat);
            Send(heartbeat);

            sf::Int32 timestamp;
            packet >> timestamp;
            SetTime(sf::milliseconds(timestamp));
            m_lastHeartbeat = m_serverTime;
        } else if (m_packetHandler) {
            m_packetHandler(id, packet, this);
        }
    }
    std::cout << "Listening stopped." << std::endl;
}

void TcpClient::Update(const sf::Time& l_time) {
    m_serverTime += l_time;
    if (m_serverTime.asMilliseconds() < 0) {
        m_serverTime -= sf::milliseconds((sf::Int32)Network::HighestTimestamp);
        m_lastHeartbeat = m_serverTime;
        return;
    }

    if (m_serverTime.asMilliseconds() - m_lastHeartbeat.asMilliseconds() >= (sf::Int32)Network::ClientTimeout) {
        Disconnect();
        std::cout << "Client has left the server." << std::endl;
    }
}

bool TcpClient::Disconnect() {
    if (!m_connected) return false;
    sf::Packet packet;
    StampPacket(PacketType::Disconnect, packet);
    Send(packet);
    m_socket.disconnect();
    m_connected = false;
    return true;
}

bool TcpClient::Send(sf::Packet& l_packet) {
    if (!m_connected) return false;
    if (m_socket.send(l_packet) != sf::Socket::Done) {
        std::cout << "Failed sending a packet to the server." << std::endl;
        return false;
    }
    return true;
}

void TcpClient::Setup(void(*l_handler)(const PacketID&, sf::Packet&, TcpClient*)) {
    m_packetHandler = std::bind(l_handler, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);
}

const sf::Time& TcpClient::GetTime() const { return m_serverTime; }

const sf::Time& TcpClient::GetLastHeartbeat() const { return m_lastHeartbeat; }

void TcpClient::SetTime(const sf::Time& l_time) { m_serverTime = l_time; }

void TcpClient::SetServerInformation(const sf::IpAddress& l_ip, const PortNumber& l_port) {
    m_serverIp = l_ip;
    m_serverPort = l_port;
}

void TcpClient::SetPlayerName(const std::string& l_name) { m_playerName = l_name; }

bool TcpClient::IsConnected() const { return m_connected; }

sf::Mutex& TcpClient::GetMutex() { return m_mutex; }

void TcpClient::UnregisterPacketHandler() { m_packetHandler = nullptr; }