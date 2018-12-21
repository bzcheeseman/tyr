cmake_minimum_required(VERSION 3.10)
project(tyr)

add_subdirectory(codegen)
add_subdirectory(frontend)
add_subdirectory(runtime)

set(ALL_CXX_SOURCE_FILES ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)
