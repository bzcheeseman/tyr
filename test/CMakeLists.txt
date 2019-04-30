find_package(tyr REQUIRED
        HINTS ${CMAKE_CURRENT_SOURCE_DIR}/install/lib/cmake)

set(CMAKE_CXX_FLAGS "-O3 -g -flto")

if(APPLE)
    set(PYTHON_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/../venv/bin/python)
else()
    set(PYTHON_EXECUTABLE python3)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

# C test
tyr_generate_obj(TYR_HDRS TYR_SRCS "-file-utils;-base64" ${CMAKE_CURRENT_SOURCE_DIR}/path.tyr)
set(SOURCES ${TYR_HDRS} ${TYR_SRCS} c/integration_test.cpp)
add_executable(c_test EXCLUDE_FROM_ALL ${SOURCES})
target_include_directories(c_test PUBLIC ${TYR_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Override test
add_library(my_allocator ${CMAKE_CURRENT_SOURCE_DIR}/override/allocator.c)

tyr_generate_llvm(TYR_OVR_HDRS TYR_OVR_SRCS "-file-utils;-base64;-malloc-name=my_alloc;-realloc-name=my_realloc;-free-name=my_free" ${CMAKE_CURRENT_SOURCE_DIR}/path.tyr)
set(OVR_SOURCES ${TYR_OVR_HDRS} ${TYR_OVR_SRCS} c/integration_test.cpp)
add_executable(c_override_test EXCLUDE_FROM_ALL ${OVR_SOURCES})
target_include_directories(c_override_test PUBLIC ${TYR_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(c_override_test PUBLIC my_allocator)
