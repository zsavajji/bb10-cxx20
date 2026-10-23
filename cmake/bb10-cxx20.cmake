# CMake toolchain file for BB10 (QNX 6.6, ARMv7) with C++20: bb10-clang and
# bb10-clang++ hold every compile and link flag.
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=/opt/bb10-cxx20/share/bb10-cxx20.cmake ...
set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 6.6.0)
set(CMAKE_SYSTEM_PROCESSOR armv7)

get_filename_component(_bb10_cxx20 "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(CMAKE_C_COMPILER "${_bb10_cxx20}/bin/bb10-clang")
set(CMAKE_CXX_COMPILER "${_bb10_cxx20}/bin/bb10-clang++")
set(CMAKE_ASM_COMPILER "${_bb10_cxx20}/bin/bb10-clang")
# clang identifies itself; the QNX platform files would otherwise expect qcc.
set(CMAKE_C_COMPILER_ID Clang)
set(CMAKE_CXX_COMPILER_ID Clang)

set(_bb10_binutils /opt/bb10-toolchain/bin/arm-unknown-nto-qnx6.5.0eabi)
set(CMAKE_AR "${_bb10_binutils}-ar" CACHE FILEPATH "BB10 archiver")
set(CMAKE_RANLIB "${_bb10_binutils}-ranlib" CACHE FILEPATH "BB10 ranlib")
set(CMAKE_STRIP "${_bb10_binutils}-strip" CACHE FILEPATH "BB10 strip")
set(CMAKE_NM "${_bb10_binutils}-nm" CACHE FILEPATH "BB10 nm")
set(CMAKE_OBJCOPY "${_bb10_binutils}-objcopy" CACHE FILEPATH "BB10 objcopy")

set(CMAKE_FIND_ROOT_PATH /opt/bb10-sdk/target/qnx6 /opt/bb10-sdk/target/qnx6/armle-v7 "${_bb10_cxx20}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
