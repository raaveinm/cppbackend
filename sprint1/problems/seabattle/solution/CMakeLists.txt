cmake_minimum_required(VERSION 3.11)

# Проект называется Hello и написан на C++
project(Seabattle CXX)
# Исходый код будет компилироваться с поддержкой стандарта С++ 20
set(CMAKE_CXX_STANDARD 20)

set(CONAN_SETUP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../conan_clion_setup.cmake)

if (EXISTS ${CONAN_SETUP_FILE})
  include(${CONAN_SETUP_FILE})
else ()
  message("conan_clion_setup not found, skipping")
endif ()

# Подключаем сгенерированный скрипт conanbuildinfo.cmake, созданный Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# Выполняем макрос из conanbuildinfo.cmake, который настроит СMake на работу с библиотеками, установленными Conan
conan_basic_setup()


include_directories(${Boost_INCLUDE_DIRS})

# Платформы вроде linux требуют подключения библиотеки pthread для
# поддержки стандартных потоков.
# Следующие две строки подключат эту библиотеку на таких платформах
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Проект содержит единственный исходный файл src/main.cpp
add_executable(seabattle src/main.cpp src/seabattle.h)
# Просим компоновщик подключить библиотеку для поддержки потоков
target_link_libraries(seabattle PRIVATE Threads::Threads)
