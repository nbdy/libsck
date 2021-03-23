cmake_minimum_required(VERSION 3.17)
project(libtcpsh)

set(CMAKE_CXX_STANDARD 17)

include_directories(ohlog)

add_executable(libtcpsh main.cpp tcpsh.h)

add_executable(example example/main.cpp example/MyShell.h tcpsh.h)