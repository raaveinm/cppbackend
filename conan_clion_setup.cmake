set(ENV{CMAKE_POLICY_VERSION_MINIMUM} "3.5")

set(RUN_CONAN FALSE)
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    set(RUN_CONAN TRUE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt" AND "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt" IS_NEWER_THAN "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    set(RUN_CONAN TRUE)
endif()

if(RUN_CONAN)
    message(STATUS "CLion: Running conan install from virtual environment...")

    if(CMAKE_CXX_COMPILER_VERSION)
        string(REGEX MATCH "^[0-9]+" GCC_MAJOR_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
    else()
        set(GCC_MAJOR_VERSION "11")
    endif()

    set(CONAN_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/.venv/bin/conan")

    execute_process(
            COMMAND "${CONAN_EXECUTABLE}" install "${CMAKE_CURRENT_SOURCE_DIR}"
            -s build_type=${CMAKE_BUILD_TYPE}
            -s compiler=gcc
            -s compiler.version=${GCC_MAJOR_VERSION}
            -s compiler.libcxx=libstdc++11
            --build=missing
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            RESULT_VARIABLE CONAN_RES
    )
    if(NOT CONAN_RES EQUAL 0)
        message(FATAL_ERROR "Conan installation failed!")
    endif()
endif()