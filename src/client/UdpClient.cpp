#include "UdpClient.h"

UdpClient::UdpClient() : m_listenThread(&UdpClient::Listen, this), m_connected(false) {}

UdpClient::~UdpClient() { m_socket.unbind(); }

bool UdpClient::Connect() {
    if (m_connected) return false;
    m_socket.bind(sf::Socket::AnyPort);
    std::cout << "Bound client to port " << m_socket.getLocalPort() << std::endl;
    sf::Packet packet;
    StampPacket(PacketType::Connect, packet);
    packet << m_playerName;
    if (m_socket.send(packet, m_serverIp, m_serverPort) != sf::Socket::Done) { m_socket.unbind(); return false; }
    packet.clear();
    m_socket.setBlocking(false);
    sf::IpAddress recvIP;
    PortNumber recvPORT;
    sf::Clock clock;
    clock.restart();
    while (clock.getElapsedTime().asMilliseconds() < CONNECT_TIMEOUT) {
        packet.clear();
        sf::Socket::Status status = m_socket.receive(packet, recvIP, recvPORT);
        if (status != sf::Socket::Done) continue;
        if (recvIP != m_serverIp) continue;

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
    std::cout << "Failed connecting to the server " << m_serverIp << ":" << m_serverPort << std::endl;
    m_socket.setBlocking(true);
    m_socket.unbind();
    return false;
}

void UdpClient::Listen() {
    sf::IpAddress recvIP;
    PortNumber recvPORT;
    sf::Packet packet;

    std::cout << "Beginning to listen..." << std::endl;
    while (m_connected) {
        packet.clear();
        sf::Socket::Status status = m_socket.receive(packet, recvIP, recvPORT);
        
        if (status != sf::Socket::Done) {
            if (m_connected) {
                std::cout << "Failed receiving a packet from " << recvIP << ":" << recvPORT 
                    << ". Status code: " << status << std::endl;
                continue;
            } else {
                std::cout << "Socket unbound." << std::endl;
                break;
            }
        }

        if (recvIP != m_serverIp) {
            std::cout << "Invalid packet received: not from the server." << std::endl;
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

void UdpClient::Update(const sf::Time& l_time) {
    if (!m_connected) return;
    m_serverTime += l_time;
    if (m_serverTime.asMilliseconds() < 0) {
        m_serverTime -= sf::milliseconds((sf::Int32)Network::HighestTimestamp);
        m_lastHeartbeat = m_serverTime;
        return;
    }

    if (m_serverTime.asMilliseconds() - m_lastHeartbeat.asMilliseconds() >= (sf::Int32)Network::ClientTimeout) {
        Disconnect();
        std::cout << "Client has timed out." << std::endl;
    }
}

void UdpClient::Setup(void(*l_handler)(const PacketID&, sf::Packet&, UdpClient*)) {
    m_packetHandler = std::bind(l_handler, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);
}

bool UdpClient::Disconnect() {
    if (!m_connected) return false;
    sf::Packet packet;
    StampPacket(PacketType::Disconnect, packet);
    sf::Socket::Status status = m_socket.send(packet, m_serverIp, m_serverPort);
    m_socket.unbind();
    m_connected = false;
    UnregisterPacketHandler();
    if (status != sf::Socket::Done) return false;
    return true;
}

const sf::Time& UdpClient::GetTime() const { return m_serverTime; }

const sf::Time& UdpClient::GetLastHeartbeat() const { return m_lastHeartbeat; }

void UdpClient::SetTime(const sf::Time& l_time) { m_serverTime = l_time; }

void UdpClient::SetServerInformation(const sf::IpAddress& l_ip, const PortNumber& l_port) {
    m_serverIp = l_ip;
    m_serverPort = l_port;
}

bool UdpClient::Send(sf::Packet& l_packet) {
    if (!m_connected) return false;
    if (m_socket.send(l_packet, m_serverIp, m_serverPort) != sf::Socket::Done) {
        std::cout << "Failed sending packet!" << std::endl;
        return false;
    }
    return true;
}

void UdpClient::SetPlayerName(const std::string& l_name) { m_playerName = l_name; }

bool UdpClient::IsConnected() const { return m_connected; }

void UdpClient::UnregisterPacketHandler() { m_packetHandler = nullptr; }

sf::Mutex& UdpClient::GetMutex() { return m_mutex; }

const std::string& UdpClient::GetPlayerName() const { return m_playerName; }