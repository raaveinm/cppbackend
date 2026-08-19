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

# Static library with the game model and everything covered by the Catch2 tests.
# Networking / connection-handling code is intentionally left out: it isn't
# exercised by the tests, so it stays in the executable targets below.
add_library(game_model_lib STATIC
		src/model/model.h
		src/model/model.cpp
		src/loot_generator.h
		src/loot_generator.cpp
		src/extra_data.h
		src/extra_data.cpp
		src/geom.h
		src/collision_detector.h
		src/collision_detector.cpp
		src/loader/json_loader.h
		src/loader/json_loader.cpp
		src/loader/boost_json.cpp
		src/util/tagged.h
		src/util/token_generator.h
		src/serialization/model_serialization.h
		src/serialization/state_serialization.h
		src/serialization/state_serialization.cpp
)

target_include_directories(game_model_lib PUBLIC src)
target_link_libraries(game_model_lib PUBLIC
		Threads::Threads
		Boost::boost
		Boost::serialization
		Boost::log
)

add_executable(game_server
		src/main.cpp
		src/net/http_server.cpp
		src/net/http_server.h
		src/util/sdk.h
		src/net/request_handler.cpp
		src/net/request_handler.h
		src/util/logging.cpp
		src/util/logging.h
		src/util/ticker.h
)

target_link_libraries(game_server PRIVATE
		game_model_lib
		Boost::log_setup
		Boost::log
		Boost::program_options
)

# Tests
include(CTest)

add_executable(game_server_tests
        tests/loot_generator_tests.cpp
        tests/model_tests.cpp
        tests/collision-detector-tests.cpp
        tests/json_loader_tests.cpp
        tests/state_serialization_tests.cpp
)

target_link_libraries(game_server_tests PRIVATE
        game_model_lib
        Catch2::Catch2WithMain
)

if (DEFINED CONAN_BUILD_DIRS_CATCH2_DEBUG)
	include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake)
elseif (DEFINED CONAN_BUILD_DIRS_CATCH2)
	include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)
else ()
	include(Catch)
endif ()

catch_discover_tests(game_server_tests)
