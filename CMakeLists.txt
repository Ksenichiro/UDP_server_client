# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name
project(UDP_GT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(Client udp_client.cpp)
add_executable(Server udp_server.cpp)