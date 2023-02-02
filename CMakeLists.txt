cmake_minimum_required(VERSION 3.23)
project(GarbageCollector C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
include_directories(.)
add_compile_options(-pthread -DDEBUG)

add_library(GarbageCollector gc.c map.c)

add_executable(test test.c gc.c map.c)