cmake_minimum_required(VERSION 3.10)
project(tyr)

file(GLOB_RECURSE COMPILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE COMPILE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

set(ALL_CXX_SOURCE_FILES ${COMPILE_SOURCES} ${COMPILE_HEADERS} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_library(tyr-rt OBJECT ${COMPILE_SOURCES})
target_compile_options(tyr-rt PRIVATE -Oz -flto -emit-llvm)
target_include_directories(tyr-rt PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/tyr/rt>)
