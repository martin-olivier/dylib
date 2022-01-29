cmake_minimum_required(VERSION 3.14)
project(dylib)

set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

option(BUILD_TESTS "when set to ON, build unit tests" OFF)

include_directories(.)

if(BUILD_TESTS)

find_package(googletest QUIET)
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if(UNIX)
    add_compile_options(-Wall -Wextra -Weffc++)
elseif(WIN32)
    add_compile_options(/W4)
endif()

add_library(dynamic_lib SHARED test/dynamic_lib.cpp)
set_target_properties(dynamic_lib PROPERTIES PREFIX "")

enable_testing()

add_executable(unit_tests test/tests.cpp)

if(UNIX AND NOT APPLE)
    add_compile_options(--coverage)
	target_link_libraries(unit_tests PRIVATE gcov)
endif()

if(UNIX)
    target_link_libraries(unit_tests PRIVATE dl)
endif()

target_link_libraries(unit_tests PRIVATE gtest_main)

include(GoogleTest)

gtest_discover_tests(unit_tests)

add_dependencies(unit_tests dynamic_lib)

endif()