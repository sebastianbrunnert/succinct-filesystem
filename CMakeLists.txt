cmake_minimum_required(VERSION 3.14)
project(SuccinctFilesystem
    VERSION 1.0
    DESCRIPTION "FUSE filesystem based on FLOUDS and other succinct data structures"
    LANGUAGES CXX
)

# Some of our dependencies require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source code
add_subdirectory(src)

# Testing
find_package(GTest REQUIRED)
enable_testing()
add_subdirectory(tests)