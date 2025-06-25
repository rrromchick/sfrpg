#pragma once

enum class NetworkProtocol {
    TCP, UDP
};

using ClientID = int;
using PortNumber = unsigned short;

enum class Network { 
    HighestTimestamp = 2147483647, ClientTimeout = 10000, ServerPort = 5600, 
    UpdateDelim = -1, NullID = -1
};