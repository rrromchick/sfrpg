cmake_minimum_required(VERSION 3.28)
project(SFRPG LANGUAGES CXX C)

enable_language(CXX)

include (GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

find_package(X11 REQUIRED)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_library(common STATIC src/common/PacketTypes.h src/common/PacketTypes.cpp src/common/NetworkDefinitions.h
    src/common/sqlite3.h src/common/sqlite3.c src/common/DbConnector.h src/common/DbConnector.cpp)

add_executable(client src/client/main.cpp src/client/UdpClient.h src/client/UdpClient.cpp)

add_executable(server src/server/main.cpp src/server/UdpServer.h src/server/UdpServer.cpp)

target_link_libraries(common sfml-graphics)
target_link_libraries(common sfml-window)
target_link_libraries(common sfml-system)
target_link_libraries(common sfml-audio)
target_link_libraries(common sfml-network)

target_link_libraries(client common)
target_link_libraries(server common)

target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)

target_compile_features(client PRIVATE cxx_std_17)
target_compile_features(server PRIVATE cxx_std_17)