file(GLOB_RECURSE COMPILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE COMPILE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

set(ALL_CXX_SOURCE_FILES ${COMPILE_SOURCES} ${COMPILE_HEADERS} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

llvm_map_components_to_libnames(LLVM_LIBS all)

add_library(tyr-llvm-passes ${COMPILE_SOURCES} ${COMPILE_HEADERS})
target_compile_options(tyr-llvm-passes PUBLIC "-fno-rtti")
target_link_libraries(tyr-llvm-passes PRIVATE ${LLVM_LIBS})
target_include_directories(tyr-llvm-passes
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${LLVM_INCLUDE_DIRS}
)