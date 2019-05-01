# Find tyr - the hint is needed because it's not installed to a system dir (this just points it at the build dir)
find_package(tyr REQUIRED
        HINTS ${CMAKE_CURRENT_BINARY_DIR}/../..)

# Create the custom allocator library
add_library(custom_allocator ${CMAKE_CURRENT_SOURCE_DIR}/allocator.cpp)

# CMake can invoke tyr for you as part of the build process - in this case it's going to generate an llvm .bc file and a header file
tyr_generate_llvm(TYR_HDRS TYR_SRCS
        # It will use these args, in CMake a semicolon is the way you delineate a list
        "-file-utils;-base64;-malloc-name=custom_alloc;-realloc-name=custom_realloc;-free-name=custom_free"
        # Pass in the path to the .tyr file
        ${CMAKE_CURRENT_SOURCE_DIR}/graph.tyr)

# Some standard CMake stuff - set up executable, include directories, link things in, etc.
set(SOURCES ${TYR_HDRS} ${TYR_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/graph.cpp)
add_executable(graph_example EXCLUDE_FROM_ALL ${SOURCES})
# The files are generated in ${CMAKE_CURRENT_BINARY_DIR} so you have to include that with target_include_directories
target_include_directories(graph_example PUBLIC ${TYR_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
# Then link in the custom allocator
target_link_libraries(graph_example PUBLIC custom_allocator)

set(ALL_CXX_SOURCE_FILES ${ALL_CXX_SOURCE_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/graph.cpp ${CMAKE_CURRENT_SOURCE_DIR}/allocator.cpp  PARENT_SCOPE)