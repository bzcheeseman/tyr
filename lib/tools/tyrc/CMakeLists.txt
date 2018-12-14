cmake_minimum_required(VERSION 3.10)
project(tyr)

set(ALL_CXX_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_custom_target(tyr-rt-copy
        COMMAND cp $<TARGET_OBJECTS:tyr-rt> ${CMAKE_CURRENT_SOURCE_DIR}/tyr-rt.bc
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/tyr-rt.bc)

add_executable(tyr ${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp)
target_link_libraries(tyr tyr-codegen tyr-frontend)
add_dependencies(tyr tyr-rt-copy)

add_definitions(-DTYR_RT_BITCODE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/tyr-rt.bc")
