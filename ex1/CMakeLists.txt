cmake_minimum_required(VERSION 3.21)
project(oop_ex1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(oop_ex1
        osm.cpp
        osm.h)
