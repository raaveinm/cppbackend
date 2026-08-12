cmake_minimum_required(VERSION 3.11)

project(htmldecode CXX)
set(CMAKE_CXX_STANDARD 20)

set(CONAN_SETUP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../conan_clion_setup.cmake)

if (EXISTS ${CONAN_SETUP_FILE})
	include(${CONAN_SETUP_FILE})
else ()
	message("conan_clion_setup not found, skipping")
endif ()

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

find_package(Catch2 REQUIRED)

add_executable(htmldecode
    src/main.cpp
    src/htmldecode.h
    src/htmldecode.cpp
)

add_executable(tests
    tests/tests.cpp
    src/htmldecode.h
    src/htmldecode.cpp
)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)