cmake_minimum_required(VERSION 3.10)
project(tyr)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

set(ALL_CXX_SOURCE_FILES "")

include(FindLLVM)

add_subdirectory(src/compiler)
add_subdirectory(src/runtime)

include(FindClangFormat)