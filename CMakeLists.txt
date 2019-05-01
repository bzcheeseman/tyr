cmake_minimum_required(VERSION 3.7)
project(tyr)

# Version
set(tyr_VERSION_MAJOR 0)
set(tyr_VERSION_MINOR 1)
set(tyr_VERSION_PATCH 0)
set(tyr_VERSION ${tyr_VERSION_MAJOR}.${tyr_VERSION_MINOR}.${tyr_VERSION_PATCH})

# CMake compiler setup
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(LLVM_VERSION 8)

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

option(BUILD_INTEGRATION_TESTS "Build the integration tests" ON)
if (BUILD_INTEGRATION_TESTS)
    add_subdirectory(test)
endif (BUILD_INTEGRATION_TESTS)

option(BUILD_EXAMPLES "Build the examples" ON)
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif (BUILD_EXAMPLES)

configure_file(${CMAKE_SOURCE_DIR}/cmake/tyr-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/tyr-config.cmake @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/cmake/tyr-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/tyr-config-version.cmake @ONLY)

include(FindClangFormat)