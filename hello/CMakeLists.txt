cmake_minimum_required(VERSION 3.11)

project(Hello CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_CURRENT_SOURCE_DIR}/../conan_clion_setup.cmake)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello src/sample.cpp)
add_executable(hamburger src/hamburger.cpp)
add_executable(sync_sample src/sync_sample.cpp)
add_executable(async_server src/async_server_notes.cpp)
add_executable(boost_fs src/boost_fs.cpp)
add_executable(logger src/logger.cpp)
add_executable(hello_log src/hello_log/hello_log.cpp)

target_link_libraries(hello PRIVATE Threads::Threads)
target_link_libraries(sync_sample PRIVATE Threads::Threads)
target_link_libraries(async_server PRIVATE Threads::Threads)
target_link_libraries(hello_log PRIVATE Boost::log Boost::log_setup Threads::Threads)