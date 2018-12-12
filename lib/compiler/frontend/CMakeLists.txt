cmake_minimum_required(VERSION 3.10)
project(tyr)

llvm_map_components_to_libnames(LLVM_LIBS all)

file(GLOB_RECURSE COMPILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE COMPILE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

set(ALL_CXX_SOURCE_FILES ${COMPILE_SOURCES} ${COMPILE_HEADERS} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_library(tyr-frontend STATIC ${COMPILE_SOURCES} ${COMPILE_HEADERS})
target_compile_options(tyr-frontend PUBLIC "-fno-rtti")
target_link_libraries(tyr-frontend PUBLIC ${LLVM_LIBS} tyr-codegen)
target_include_directories(tyr-frontend
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${LLVM_INCLUDE_DIRS}
)
