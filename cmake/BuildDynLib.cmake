cmake_minimum_required(VERSION 3.14)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

add_library(dynlib SHARED
        test/myDynLib.cpp)

set_target_properties(dynlib PROPERTIES PREFIX "")