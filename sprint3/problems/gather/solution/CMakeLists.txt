cmake_minimum_required(VERSION 3.15)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

set(Boost_NO_BOOST_CMAKE OFF)
set(Boost_NO_SYSTEM_PATHS ON)
set(CONAN_SETUP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../conan_clion_setup.cmake)

if (EXISTS ${CONAN_SETUP_FILE})
	include(${CONAN_SETUP_FILE})
else ()
	message("conan_clion_setup not found, skipping")
endif ()

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

find_package(Boost REQUIRED)
find_package(Catch2 REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(collision_detection_lib STATIC
	src/collision_detector.h
	src/collision_detector.cpp
)

target_link_libraries(collision_detection_lib PUBLIC boost::boost Threads::Threads)

add_executable(collision_detection_tests
	tests/collision-detector-tests.cpp
)

target_link_libraries(collision_detection_tests Catch2::Catch2WithMain collision_detection_lib)
