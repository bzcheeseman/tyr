add_subdirectory(ir)
add_subdirectory(passes)
add_subdirectory(codegen)
add_subdirectory(frontend)
add_subdirectory(runtime)

set(ALL_CXX_SOURCE_FILES ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)
