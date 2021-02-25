cmake_minimum_required(VERSION 3.10)

project(my_reclaimer)

set(CMAKE_CXX_STANDARD 14)

link_libraries(pthread atomic)

add_executable(my_reclaimer main.cpp)