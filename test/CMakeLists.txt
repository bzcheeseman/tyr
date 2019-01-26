find_package(tyr REQUIRED
        HINTS ${CMAKE_CURRENT_SOURCE_DIR}/install/lib/cmake)

set(CMAKE_CXX_FLAGS "-O3 -flto ${CMAKE_CXX_FLAGS}")

if(APPLE)
    set(PYTHON_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/../venv/bin/python)
else()
    set(PYTHON_EXECUTABLE python3)
endif()

# C test
tyr_generate_llvm(TYR_HDRS TYR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/path.tyr)
set(SOURCES ${TYR_HDRS} ${TYR_SRCS} c/integration_test.cpp)
add_executable(c_test EXCLUDE_FROM_ALL ${SOURCES})

# Python test
tyr_generate_python(TYR_PY TYR_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/python ${CMAKE_CURRENT_SOURCE_DIR}/path.tyr)
add_custom_target(python_test
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/integration_test.py
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/python/integration_test.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${TYR_PY} ${TYR_LIBS})

# Cross language test
# C part
tyr_generate_c(TYR_HDRS TYR_OBJS ${CMAKE_CURRENT_SOURCE_DIR}/path.tyr)
set(SOURCES ${TYR_HDRS} ${TYR_OBJS} x-lang/integration_test.cpp)
add_executable(x_lang_c_test EXCLUDE_FROM_ALL ${SOURCES})
target_include_directories(x_lang_c_test PRIVATE ${TYR_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Python part
tyr_generate_python(TYR_PY TYR_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/x-lang ${CMAKE_CURRENT_SOURCE_DIR}/path.tyr)
add_custom_target(x_lang_test
        COMMAND x_lang_c_test
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/integration_test.py
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/python/integration_test.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${TYR_PY} ${TYR_LIBS} x_lang_c_test)
