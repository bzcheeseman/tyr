file(GLOB_RECURSE COMPILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE COMPILE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

set(ALL_CXX_SOURCE_FILES ${COMPILE_SOURCES} ${COMPILE_HEADERS} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_library(tyr-passes STATIC ${COMPILE_SOURCES} ${COMPILE_HEADERS})
target_compile_options(tyr-passes PUBLIC "-fno-rtti")
target_link_libraries(tyr-passes PUBLIC LLVM tyr-ir)
target_include_directories(tyr-passes
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${LLVM_INCLUDE_DIRS}
)
