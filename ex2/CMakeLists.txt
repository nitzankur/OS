cmake_minimum_required(VERSION 3.21)
project(ex2 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(ex2
        demo_itimer.c
        demo_jmp.c
        demo_singInt_handler.c
        uthreads.h)
