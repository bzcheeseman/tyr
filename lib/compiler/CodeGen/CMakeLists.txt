file(GLOB_RECURSE COMPILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE COMPILE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

set(ALL_CXX_SOURCE_FILES ${COMPILE_SOURCES} ${COMPILE_HEADERS} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_definitions(-DTYR_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}")

add_library(tyr-codegen STATIC ${COMPILE_SOURCES} ${COMPILE_HEADERS})
target_compile_options(tyr-codegen PUBLIC "-fno-rtti")
target_link_libraries(tyr-codegen PUBLIC LLVM tyr-ir tyr-llvm-passes)
target_include_directories(tyr-codegen
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${LLVM_INCLUDE_DIRS}
)
