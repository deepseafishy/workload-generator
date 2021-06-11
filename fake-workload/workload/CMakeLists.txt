cmake_minimum_required(VERSION 3.10)

project(workload)

find_package(Boost 1.40 COMPONENTS program_options REQUIRED)

add_executable(workload workload/workload.cpp)

target_link_libraries(workload PUBLIC libfakework Boost::program_options)
