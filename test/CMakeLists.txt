find_package(tyr REQUIRED
        HINTS ${CMAKE_CURRENT_SOURCE_DIR}/install/lib/cmake)

set(CMAKE_CXX_FLAGS "-O3 -flto ${CMAKE_CXX_FLAGS}")

if(APPLE)
    set(PYTHON_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/../venv/bin/python)
else()
    set(PYTHON_EXECUTABLE python3)
endif()

# C test
tyr_generate_obj(TYR_HDRS TYR_SRCS "file-utils" ${CMAKE_CURRENT_SOURCE_DIR}/path.tyr)
set(SOURCES ${TYR_HDRS} ${TYR_SRCS} c/integration_test.cpp)
add_executable(c_test EXCLUDE_FROM_ALL ${SOURCES})
target_include_directories(c_test PUBLIC ${TYR_INCLUDE_DIR})
