include(CMakeForceCompiler)

# cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm-eabi.cmake ../

set ( CMAKE_SYSTEM_NAME          Generic )
set ( CMAKE_SYSTEM_PROCESSOR     ARM )

set ( TC_PATH "/usr/bin/" )

set ( CROSS_COMPILE arm-none-eabi- )

set ( CMAKE_C_COMPILER ${TC_PATH}${CROSS_COMPILE}gcc )
set ( CMAKE_CXX_COMPILER ${TC_PATH}${CROSS_COMPILE}g++ )
set ( CMAKE_LINKER ${TC_PATH}${CROSS_COMPILE}ld
      CACHE FILEPATH "The toolchain linker command " FORCE )
set ( CMAKE_OBJCOPY ${TC_PATH}${CROSS_COMPILE}objcopy
      CACHE FILEPATH "The toolchain objcopy command " FORCE )

set ( EXE_LINKER_FLAGS "${EXE_LINKER_FLAGS} -T /home/paso/development/tools/src/ld/LPC4337.ld")
set ( EXE_LINKER_FLAGS "${EXE_LINKER_FLAGS} --specs=rdimon.specs -lrdimon")

set ( CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set ( C_FLAGS "${C_FLAGS} -mcpu=cortex-m4")
set ( C_FLAGS "${C_FLAGS} -mfpu=fpv4-sp-d16")
set ( C_FLAGS "${C_FLAGS} -mfloat-abi=hard")
set ( C_FLAGS "${C_FLAGS} -mthumb")

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${C_FLAGS} CACHE STRING "Cortex M4 C Flags")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${C_FLAGS} CACHE STRING "Cortex M4 C++ Flags")
set(CMAKE_ASM_FLAGS ${CMAKE_ASM_FLAGS} ${C_FLAGS} CACHE STRING "Cortex M4 asm Flags")
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} ${EXE_LINKER_FLAGS} CACHE STRING "Cortex M4 Linker Flags")
