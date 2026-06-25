cmake_minimum_required(VERSION 3.11)

project(Hello CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_CURRENT_SOURCE_DIR}/../conan_clion_setup.cmake)

# enable generated Conan script conanbuildinfo.cmake
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#  conanbuildinfo.cmake library management macros
conan_basic_setup()

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello src/sample.cpp)
add_executable(hamburger src/hamburger.cpp)
add_executable(sync_sample src/sync_sample.cpp)

target_link_libraries(hello PRIVATE Threads::Threads)
target_link_libraries(sync_sample PRIVATE Threads::Threads)