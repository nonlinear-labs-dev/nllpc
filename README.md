# Building:

`mkdir -p build && cd build`
`cmake ../  -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug`
`make`

# Debugging

`openocd -f openocd/nonlinear-lpc.cfg`

