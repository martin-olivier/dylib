cmake_minimum_required(VERSION 3.11...3.31)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/dylib.hpp" DYLIB_HEADER)

foreach(part MAJOR MINOR PATCH)
    if(NOT DYLIB_HEADER MATCHES "#define DYLIB_VERSION_${part} ([0-9]+)")
        message(FATAL_ERROR "Could not parse DYLIB_VERSION_${part} from include/dylib.hpp")
    endif()
    set(DYLIB_VERSION_${part} ${CMAKE_MATCH_1})
endforeach()

project(dylib
    VERSION ${DYLIB_VERSION_MAJOR}.${DYLIB_VERSION_MINOR}.${DYLIB_VERSION_PATCH}
    LANGUAGES CXX
)

include(GNUInstallDirs)

add_library(dylib)

target_sources(dylib PRIVATE
    src/dylib.cpp
    src/symbols.cpp
    src/demangle.cpp
    src/format.cpp
)

target_include_directories(dylib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(dylib PUBLIC cxx_std_11)

target_link_libraries(dylib PRIVATE ${CMAKE_DL_LIBS})

if(BUILD_SHARED_LIBS AND WIN32)
    set_target_properties(dylib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

install(
    TARGETS dylib
    EXPORT dylibTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT dylibTargets
    FILE dylibTargets.cmake
    NAMESPACE dylib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dylib
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/dylibConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dylibConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dylibConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dylib
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/dylibConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/dylibConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dylib
)

option(DYLIB_BUILD_TESTS "When set to ON, build unit tests" OFF)
option(DYLIB_WARNING_AS_ERRORS "Treat warnings as errors" OFF)

# Applies the project warning flags to a target.
#
# These are set per target rather than through add_compile_options, which only
# affects targets created after the call: the dylib target is created at the top
# of this file, so a directory level setting would leave the sources of the
# library itself, the ones that matter most here, unchecked.
function(dylib_set_warnings target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Weffc++)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    endif()

    if(DYLIB_WARNING_AS_ERRORS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /WX)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

if(DYLIB_BUILD_TESTS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

    find_package(googletest QUIET)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    dylib_set_warnings(dylib)

    add_library(dynamic_lib SHARED tests/lib.cpp)
    dylib_set_warnings(dynamic_lib)
    if(WIN32 AND MINGW)
        set_target_properties(dynamic_lib PROPERTIES PREFIX "")
    endif()

    enable_testing()

    add_executable(unit_tests tests/tests.cpp)
    add_dependencies(unit_tests dynamic_lib)
    target_link_libraries(unit_tests PRIVATE gtest_main dylib)
    dylib_set_warnings(unit_tests)

    # Coverage has to instrument the library sources, not only the test driver.
    if(UNIX AND NOT APPLE)
        target_compile_options(dylib PRIVATE -fprofile-arcs -ftest-coverage)
        target_link_libraries(dylib PRIVATE gcov)
        target_compile_options(unit_tests PRIVATE -fprofile-arcs -ftest-coverage)
        target_link_libraries(unit_tests PRIVATE gcov)
    endif()

    include(GoogleTest)
    gtest_discover_tests(unit_tests PROPERTIES DISCOVERY_TIMEOUT 600 WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
