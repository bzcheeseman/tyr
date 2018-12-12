cmake_minimum_required(VERSION 3.10)
project(tyr)

llvm_map_components_to_libnames(LLVM_LIBS all)

file(GLOB_RECURSE COMPILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE COMPILE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

set(ALL_CXX_SOURCE_FILES ${COMPILE_SOURCES} ${COMPILE_HEADERS} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_library(tyr-codegen STATIC ${COMPILE_SOURCES} ${COMPILE_HEADERS})
target_compile_options(tyr-codegen PUBLIC "-fno-rtti")
target_link_libraries(tyr-codegen PUBLIC ${LLVM_LIBS})
target_include_directories(tyr-codegen
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${LLVM_INCLUDE_DIRS}
)
