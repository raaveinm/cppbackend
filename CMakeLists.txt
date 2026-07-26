cmake_minimum_required(VERSION 3.15)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

set(Boost_NO_BOOST_CMAKE OFF)
set(Boost_NO_SYSTEM_PATHS ON)
set(CONAN_SETUP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/conan_clion_setup.cmake)

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
		src/net/http_server.cpp
		src/net/http_server.h
		src/util/sdk.h
		src/model/model.h
		src/model/model.cpp
		src/util/tagged.h
		src/loader/boost_json.cpp
		src/loader/json_loader.h
		src/loader/json_loader.cpp
		src/net/request_handler.cpp
		src/net/request_handler.h
		src/util/logging.cpp
		src/util/logging.h
		src/model/player.h
		src/util/token_generator.h
		src/util/ticker.h
)

target_link_libraries(game_server PRIVATE
		Threads::Threads
		Boost::log_setup
		Boost::log
		boost::boost
)