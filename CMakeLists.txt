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

add_subdirectory(SQLiteCpp)

add_library(common STATIC src/common/PacketTypes.h src/common/PacketTypes.cpp src/common/NetworkDefinitions.h)

add_executable(client src/client/main.cpp src/client/Client.h src/client/Client.cpp)

add_executable(server src/server/main.cpp src/server/Server.h src/server/Server.cpp)

target_link_libraries(common PRIVATE sfml-graphics)
target_link_libraries(common PRIVATE sfml-window)
target_link_libraries(common PRIVATE sfml-system)
target_link_libraries(common PRIVATE sfml-audio)
target_link_libraries(common PRIVATE sfml-network)
target_link_libraries(common PRIVATE SQLiteCpp sqlite3 pthread dl)

target_link_libraries(client PRIVATE common)
target_link_libraries(server PRIVATE common)
target_link_libraries(server PRIVATE SQLiteCpp sqlite3 pthread dl)

target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/common)

target_compile_features(client PRIVATE cxx_std_17)
target_compile_features(server PRIVATE cxx_std_17)