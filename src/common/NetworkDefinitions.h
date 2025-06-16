#pragma once

using ClientID = int;
using PortNumber = unsigned short;

enum class Network {
    HighestTimestamp = 2147483647, ServerPort = 5600, ClientTimeout = 10000,
    NullID = -1, PlayerUpdateDelim = -1
};