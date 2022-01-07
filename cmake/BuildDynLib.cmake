cmake_minimum_required(VERSION 3.14)

add_library(dynlib SHARED test/myDynLib.cpp)

target_include_directories(dynlib PUBLIC .)

set_target_properties(dynlib PROPERTIES PREFIX "")