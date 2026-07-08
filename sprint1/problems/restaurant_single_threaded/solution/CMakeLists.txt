cmake_minimum_required(VERSION 3.11)

project(restaurant CXX)
set(CMAKE_CXX_STANDARD 20)

set(CONAN_SETUP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../conan_clion_setup.cmake)

if (EXISTS ${CONAN_SETUP_FILE})
  include(${CONAN_SETUP_FILE})
else ()
  message("conan_clion_setup not found, skipping")
endif ()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(restaurant src/main.cpp)
target_link_libraries(restaurant PRIVATE Threads::Threads)
