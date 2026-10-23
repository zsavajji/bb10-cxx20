# Configures with the toolchain file and builds a C++20 program, a static
# and a shared library: cmake -DCMAKE_TOOLCHAIN_FILE=.../bb10-cxx20.cmake
cmake_minimum_required(VERSION 3.20)
project(bb10_cxx20_cmake_test C CXX)

# CMake learns these by compiling and linking a probe with the driver.
if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR NOT CMAKE_CXX_IMPLICIT_LINK_LIBRARIES)
    message(FATAL_ERROR "compiler ABI detection failed")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
add_library(greeting STATIC greeting.cpp)
add_library(counter SHARED counter.c)
add_executable(cmake-test main.cpp)
target_link_libraries(cmake-test PRIVATE greeting counter Threads::Threads)
