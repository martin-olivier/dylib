cmake_minimum_required(VERSION 3.14)
project(DyLib)

set(CMAKE_VERBOSE_MAKEFILE off)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

include(FindGTest.cmake)

if (UNIX)
    add_compile_options(-Wall -Wextra -Weffc++)
else ()
    add_compile_options(/W4)
endif()

include(BuildDynLib.cmake)

if (UNIX AND NOT APPLE)
    add_compile_options(--coverage)
endif()

enable_testing()

add_executable(unit_tests
        test/tests.cpp)

target_include_directories(unit_tests PRIVATE .)

if (UNIX AND NOT APPLE)
    target_link_libraries(unit_tests PRIVATE
            gcov
    )
endif()

target_link_libraries(unit_tests PRIVATE
        dl
        gtest_main
        pthread
)

include(GoogleTest)

gtest_discover_tests(unit_tests)

add_dependencies(unit_tests dynlib)
