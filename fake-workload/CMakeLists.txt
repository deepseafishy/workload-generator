cmake_minimum_required(VERSION 3.10)
project(fakework)

add_compile_options(-Wall -Wextra -pedantic -Werror)
add_subdirectory(libfakework)
add_subdirectory(workload)
