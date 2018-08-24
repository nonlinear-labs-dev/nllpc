cmake_minimum_required(VERSION 3.2)

project(c15_lpc_m0)

set ( CMAKE_TOOLCHAIN_FILE "cmake/toolchain-arm-eabi.cmake")

add_compile_options("-DCORE_M4")
add_compile_options("-DC15_VERSION_4")

file(GLOB_RECURSE SOURCES ./*.c)

include_directories("nl_lib/nl_lib_com")
include_directories("nl_lib/nl_lib_m4/src")


add_executable(${PROJECT_NAME} ${SOURCES})

