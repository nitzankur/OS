cmake_minimum_required(VERSION 3.16.3)
project(ex2)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(ex2
        demo_itimer.c
        demo_jmp.c
        demo_singInt_handler.c
        uthreads.cpp
        uthreads.h
        basic_test.cpp)
