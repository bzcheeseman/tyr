cmake_minimum_required(VERSION 3.10)
project(tyr)

set(ALL_CXX_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_executable(tyr ${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp)
target_link_libraries(tyr tyr-codegen tyr-frontend)
add_dependencies(tyr tyr-rt-core)

add_definitions(-DTYR_RT_BITCODE_FILE="${CMAKE_BINARY_DIR}/tyr-rt-core.bc")

install(TARGETS tyr EXPORT tyrc RUNTIME DESTINATION bin)
install(EXPORT tyrc DESTINATION lib/tyr)
