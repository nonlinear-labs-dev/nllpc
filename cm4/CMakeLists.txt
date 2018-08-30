cmake_minimum_required(VERSION 3.2)

project(c15_lpc_m4)

set (CPU "cortex-m4")

# Optimisation / CPU and Warnings
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb -mcpu=${CPU} -Wall")

# Autoremove unused code
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections -Wl,--gc-sections -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

add_compile_options("-DCORE_M4")
add_compile_options("-DC15_VERSION_4")

file(GLOB_RECURSE SOURCES ./*.c ../lib/*.c)

include_directories ("../lib")
include_directories (".")

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-L${PROJECT_SOURCE_DIR}/ld -TLPC4337_M4.ld")
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS}")
