cmake_minimum_required(VERSION 3.2)

project(c15_lpc_m0)

set (CPU "cortex-m4")



# Optimisation / CPU and Warnings
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb -mcpu=${CPU} -Wall")

# Autoremove unused code
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections -Wl,--gc-sections")



add_compile_options("-DCORE_M4")
add_compile_options("-DC15_VERSION_4")
#add_compile_options("-DDONT_RESET_ON_RESTART")

file(GLOB_RECURSE SOURCES ./*.c)

include_directories("nl_lib/nl_lib_com")
include_directories("nl_lib/nl_lib_m4/src")

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-L${PROJECT_SOURCE_DIR}/ld -TLPC4337_M4.ld")

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS}")


message("Linker is: ${CMAKE_C_LINK_EXECUTABLE}")

