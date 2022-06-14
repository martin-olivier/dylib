cmake_minimum_required(VERSION 3.14)

project(dylib CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dylib INTERFACE)
target_include_directories(dylib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(UNIX)
    target_link_libraries(dylib INTERFACE dl)
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

    option(BUILD_TESTS "when set to ON, build unit tests" OFF)

    if(BUILD_TESTS)
        find_package(googletest QUIET)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)

        if(UNIX)
            add_compile_options(-Wall -Wextra -Weffc++)
        elseif(WIN32)
            add_compile_options(/W4)
        endif()

        if(UNIX AND NOT APPLE)
            SET(CMAKE_CXX_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
            SET(CMAKE_C_FLAGS "-g -O0 -Wall -W -fprofile-arcs -ftest-coverage")
            SET(CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
        endif()

        add_library(dynamic_lib SHARED tests/lib.cpp)

        enable_testing()

        add_executable(unit_tests tests/tests.cpp)
        target_link_libraries(unit_tests PRIVATE gtest_main)
        target_link_libraries(unit_tests PRIVATE dylib)

        if(UNIX AND NOT APPLE)
            target_link_libraries(unit_tests PRIVATE gcov)
        endif()

        include(GoogleTest)
        gtest_discover_tests(unit_tests WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    endif()
endif()
