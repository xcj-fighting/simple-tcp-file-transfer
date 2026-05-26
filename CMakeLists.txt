cmake_minimum_required(VERSION 3.10)

project(simple_socket_file_transfer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(client file_client.cpp)
add_executable(server file_server.cpp)