add_subdirectory(IR)
add_subdirectory(CodeGen)
add_subdirectory(Parser)
add_subdirectory(Runtime)
add_subdirectory(llvm)

set(ALL_CXX_SOURCE_FILES ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)
