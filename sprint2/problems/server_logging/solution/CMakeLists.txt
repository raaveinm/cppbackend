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

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
		src/main.cpp
		src/http_server.cpp
		src/http_server.h
		src/sdk.h
		src/model.h
		src/model.cpp
		src/tagged.h
		src/boost_json.cpp
		src/json_loader.h
		src/json_loader.cpp
		src/request_handler.cpp
		src/request_handler.h
		src/logging.cpp
		src/logging.h
)

target_link_libraries(game_server PRIVATE
		Threads::Threads
		Boost::log_setup
		Boost::log
		boost::boost
)