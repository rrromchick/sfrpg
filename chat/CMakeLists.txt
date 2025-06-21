cmake_minimum_required(VERSION 3.28)
project(SFMLChatApp CXX)

find_package(SFML 2.6 COMPONENTS network system REQUIRED)

set(SOURCE_FILES main.cpp MessageQueue.cpp ChatServer.cpp ChatClient.cpp)

add_executable(sfml_chat_app ${SOURCE_FILES})

target_link_libraries(sfml_chat_app PRIVATE sfml-network sfml-system)

set_property(TARGET sfml_chat_app PROPERTY CXX_STANDARD 17)
set_property(TARGET sfml_chat_app PROPERTY CXX_STANDARD_REQUIRED YES)
set_property(TARGET sfml_chat_app PROPERTY CXX_EXTENSIONS NO)