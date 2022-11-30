

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

SET(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) 

set(tools /home/nick/cross-pi-gcc-10.2.0-0)
set(CMAKE_C_COMPILER   ${tools}/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/arm-linux-gnueabihf-g++)

message(using toolchain ${tools})
#set(CMAKE_SYSROOT  ${tools}/arm-linux-gnueabihf)

#set(CMAKE_SYSROOT /mnt/remotecompile/)
#set(CMAKE_CXX_FLAGS "-march=armv6")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

