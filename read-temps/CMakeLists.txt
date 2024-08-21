cmake_minimum_required(VERSION 3.5)

project(read-temps)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra")

add_executable(task-a task-a.cpp)
add_executable(task-b task-b.cpp)
