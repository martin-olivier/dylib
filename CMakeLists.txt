cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(dylib)

project(dylib VERSION 3.0.0 LANGUAGES CXX)

if(NOT "${CMAKE_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/dylib.cpp
    src/symbols.cpp
    src/demangle.cpp
    src/format.cpp
)

add_library(dylib STATIC ${SOURCES})

target_include_directories(dylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(UNIX)
    target_link_libraries(dylib PRIVATE dl)
    if(NOT APPLE)
        target_link_libraries(dylib PRIVATE elf)
    endif()
endif()

option(DYLIB_BUILD_TESTS "When set to ON, build unit tests" OFF)
option(DYLIB_WARNING_AS_ERRORS "Treat warnings as errors" OFF)

if(DYLIB_BUILD_TESTS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

    find_package(googletest QUIET)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wall -Wextra -Weffc++)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        add_compile_options(-Wall -Wextra)
    elseif(MSVC)
        add_compile_options(/W4)
    endif()

    if(DYLIB_WARNING_AS_ERRORS)
        if(MSVC)
            add_compile_options(/WX)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
            add_compile_options(-Werror)
        endif()
    endif()

    add_library(dynamic_lib SHARED tests/lib.cpp)
    if(WIN32 AND MINGW)
        set_target_properties(dynamic_lib PROPERTIES PREFIX "")
    endif()

    enable_testing()

    if(UNIX AND NOT APPLE)
        add_compile_options(-fprofile-arcs -ftest-coverage)
    endif()

    add_executable(unit_tests tests/tests.cpp)
    add_dependencies(unit_tests dynamic_lib)
    target_link_libraries(unit_tests PRIVATE gtest_main dylib)

    if(UNIX AND NOT APPLE)
        target_link_libraries(unit_tests PRIVATE gcov)
    endif()

    include(GoogleTest)
    gtest_discover_tests(unit_tests PROPERTIES DISCOVERY_TIMEOUT 600 WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
