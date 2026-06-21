cmake_minimum_required(VERSION 3.11)

project(TCP_UDP_Practice CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../conan_clion_setup.cmake)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(TCPServer tcp_server.cpp)
add_executable(TCPClient tcp_client.cpp)
add_executable(UDPServer udp_server.cpp)
add_executable(UDPClient udp_client.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(TCPServer PRIVATE Threads::Threads)
target_link_libraries(TCPClient PRIVATE Threads::Threads)
target_link_libraries(UDPServer PRIVATE Threads::Threads)
target_link_libraries(UDPClient PRIVATE Threads::Threads)