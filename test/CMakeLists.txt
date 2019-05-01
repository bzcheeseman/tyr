find_package(tyr REQUIRED HINTS ${CMAKE_CURRENT_BINARY_DIR}/..)

set(CMAKE_VERBOSE_MAKEFILE ON)

# C test
tyr_generate_obj(TYR_HDRS TYR_SRCS "-file-utils;-base64" ${CMAKE_CURRENT_SOURCE_DIR}/path.tyr)
set(SOURCES ${TYR_HDRS} ${TYR_SRCS} c/integration_test.cpp)
add_executable(c_test EXCLUDE_FROM_ALL ${SOURCES})
target_include_directories(c_test PUBLIC ${TYR_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(c_test PRIVATE -fsanitize=address)
target_compile_options(c_test PRIVATE -fsanitize=address -O0 -g)