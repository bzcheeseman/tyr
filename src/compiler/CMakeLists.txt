cmake_minimum_required(VERSION 3.10)
project(tyr-compile)

file(GLOB COMPILE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)
file(GLOB COMPILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

set(ALL_CXX_SOURCE_FILES ${COMPILE_INCLUDE} ${COMPILE_SOURCES} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_library(tyr-compile SHARED ${COMPILE_SOURCES} ${COMPILE_INCLUDE})
target_include_directories(tyr-compile
        PUBLIC
            $<INSTALL_INTERFACE:include/compile>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PUBLIC ${LLVM_INCLUDE_DIRS}
)

add_pass(PrintFunctionName ${CMAKE_CURRENT_SOURCE_DIR}/passes/PrintFunctionName.cpp)