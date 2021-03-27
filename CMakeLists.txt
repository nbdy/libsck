cmake_minimum_required(VERSION 3.17)
project(libsck)

set(CMAKE_CXX_STANDARD 17)

include_directories(ohlog)

add_executable(example main.cpp sck.h)
target_link_libraries(example pthread)