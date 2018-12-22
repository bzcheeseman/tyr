cmake_minimum_required(VERSION 3.7)
project(tyr)

# Version
set (tyr_VERSION_MAJOR 0)
set (tyr_VERSION_MINOR 1)
set (tyr_VERSION_PATCH 0)

# CMake compiler setup
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(FindLLVM)

# Setup for clang-format
set(ALL_CXX_SOURCE_FILES "")

# Add the subdirectories
add_subdirectory(lib/compiler)
add_subdirectory(lib/tools)

option(BUILD_TESTS "Build the tests" ON)

if (BUILD_TESTS)
    enable_testing()
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V)
    include(AddGTest)

    add_subdirectory(lib/test)
endif(BUILD_TESTS)

include(FindClangFormat)