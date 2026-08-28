set(ENV{CMAKE_POLICY_VERSION_MINIMUM} "3.5")

# This file is a CLion-local dev convenience (invokes the IDE's own .venv/conan).
# CI builds run conan explicitly before invoking cmake, so skip this entirely there.
if(DEFINED ENV{CI})
    return()
endif()

set(RUN_CONAN FALSE)
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    set(RUN_CONAN TRUE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt"
        AND "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt"
        IS_NEWER_THAN "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    set(RUN_CONAN TRUE)
endif()

if(RUN_CONAN)
    message(STATUS "CLion: Running conan install from virtual environment...")

    set(CONAN_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/.venv/bin/conan")

    if(NOT EXISTS "${CONAN_EXECUTABLE}")
        message(FATAL_ERROR "Could not find conan executable in your .venv!")
    endif()

    if(APPLE)
        set(PROFILE_SETTINGS
"compiler=apple-clang
compiler.version=21.0
compiler.libcxx=libc++
compiler.cppstd=${CMAKE_CXX_STANDARD}"
        )
        set(PROFILE_CONF "")
    else()
        if(CMAKE_CXX_COMPILER_VERSION)
            string(REGEX MATCH "^[0-9]+" GCC_MAJOR_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
        else()
            set(GCC_MAJOR_VERSION "11")
        endif()
        set(PROFILE_SETTINGS
"compiler=gcc
compiler.version=${GCC_MAJOR_VERSION}
compiler.libcxx=libstdc++11
compiler.cppstd=${CMAKE_CXX_STANDARD}"
        )
        set(PROFILE_CONF "")
        if(CMAKE_C_COMPILER AND CMAKE_CXX_COMPILER)
            set(PROFILE_CONF
"[conf]
tools.build:compiler_executables={\"c\":\"${CMAKE_C_COMPILER}\",\"cpp\":\"${CMAKE_CXX_COMPILER}\"}"
            )
        endif()
    endif()

    # Extra profile layered on top of "default": overrides the compiler settings to
    # match CLion's toolchain, and pins an older CMake as a tool_requires since some
    # vendored dependencies (e.g. libpqxx) ship CMake buildsystems that rely on
    # internal modules removed from newer CMake.
    set(CONAN_OVERRIDE_PROFILE "${CMAKE_BINARY_DIR}/conan_override_profile.txt")
    file(WRITE "${CONAN_OVERRIDE_PROFILE}"
"[settings]
${PROFILE_SETTINGS}
build_type=${CMAKE_BUILD_TYPE}

[tool_requires]
cmake/3.19.8

${PROFILE_CONF}
")

    execute_process(
            COMMAND "${CONAN_EXECUTABLE}" install "${CMAKE_CURRENT_SOURCE_DIR}"
            --output-folder=${CMAKE_BINARY_DIR}
            --profile:host=default
            --profile:host=${CONAN_OVERRIDE_PROFILE}
            --build=missing
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            RESULT_VARIABLE CONAN_RES
    )

    if(NOT CONAN_RES EQUAL 0)
        message(FATAL_ERROR "Conan installation failed!")
    endif()
endif()