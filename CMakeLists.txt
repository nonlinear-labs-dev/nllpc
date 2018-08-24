cmake_minimum_required(VERSION 3.2)

project(c15_lpc_m0)
add_executable(${PROJECT_NAME} "C15_M0.c")


file(GLOB_RECURSE LIBRARY_SOURCES *.c)
add_compile_options(-Wall)
