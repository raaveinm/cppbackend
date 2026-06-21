cmake_minimum_required(VERSION 3.11)

project(TCP_UDP_Practice CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../conan_clion_setup.cmake)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(HTTP_Server src/http_server.cpp)
add_executable(HTTP_Server_Single_Time src/http_server_single_time.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(HTTP_Server PRIVATE Threads::Threads)
target_link_libraries(HTTP_Server_Single_Time PRIVATE Threads::Threads)

add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)