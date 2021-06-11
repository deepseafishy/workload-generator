cmake_minimum_required(VERSION 3.10)
project(libfakework)

set(CMAKE_STATIC_LIBRARY_PREFIX "")

add_library(libfakework libfakework/fakework/memory.cpp)

target_include_directories(libfakework PUBLIC libfakework)

find_package(Threads)
target_link_libraries(libfakework ${CMAKE_THREAD_LIBS_INIT})
