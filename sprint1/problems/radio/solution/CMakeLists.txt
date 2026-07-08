cmake_minimum_required(VERSION 3.11)

project(Radio CXX)
set(CMAKE_CXX_STANDARD 20)

set(CONAN_SETUP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../conan_clion_setup.cmake)

if (EXISTS ${CONAN_SETUP_FILE})
    include(${CONAN_SETUP_FILE})
else ()
    message("conan_clion_setup not found, skipping")
endif ()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories(${CONAN_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(WIN32)
    add_definitions(-DNOMINMAX)
endif()

add_executable(radio src/main.cpp src/audio.h)
target_link_libraries(radio PRIVATE Threads::Threads)
