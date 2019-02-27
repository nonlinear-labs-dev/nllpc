# nllpc

This contains the lpc code for the c15. It can be built using [cmake](https://cmake.org/) and any [arm toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm). Therefore you'll find a [toolchainfile](cmake/arm-gcc-toolchain.cmake) in the cmake folder. For debugging, [openocd](http://openocd.org/) is used. Any [JTGA Adapter](http://openocd.org/doc/html/Debug-Adapter-Hardware.html#Debug-Adapter-Hardware) compatible to openocd could possibly be used for debugging. You might have to change the configuration file in [openocd/nonlinear-lpc.cfg](openocd/nonlinear-lpc.cfg).

OpenOCD set's up as GDB server. So you can use any GDB client, to debug. Even over network, if necessary. This process is very well [documented](https://sourceware.org/gdb/onlinedocs/gdb/Server.html)

## Building:

`mkdir -p build && cd build`
`cmake ../  -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug`
`make`

## Flash Programming:

You can use OpenOCD to program your flash. Either by using OpenOCD as a [GDB Server](http://openocd.org/doc/html/GDB-and-OpenOCD.html#programmingusinggdb) or by using OpenOCD's own [Flash Programming Commands](http://openocd.org/doc/html/Flash-Commands.html#flashprogrammingcommands)

## Debugging:

`openocd -f openocd/nonlinear-lpc.cfg`

## IDE:

Since this setup very similar to many open source IDE's way of doing things such as [eclipse with cdt plugin][https://www.eclipse.org/cdt/] or [qtcreator with baremetal plugin][https://doc.qt.io/qtcreator/creator-developing-baremetal.html]  it should be fairly easy to setup an IDE.
