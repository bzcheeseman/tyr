file(GLOB_RECURSE COMPILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE COMPILE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

set(ALL_CXX_SOURCE_FILES ${COMPILE_SOURCES} ${COMPILE_HEADERS} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
    set(CLANG_BIN ${CMAKE_C_COMPILER})
else()
    find_program(CLANG_BIN clang)
endif()

find_program(LLVM_LINK_BIN REQUIRED
        NAMES
        llvm-link-6.0
        llvm-link-7
        llvm-link

        llvm-ld-6.0
        llvm-ld-7
        llvm-ld)

set(CMAKE_LLIR_CREATE_SHARED_LIBRARY "${LLVM_LINK_BIN} -o <TARGET> <OBJECTS>")
set(CMAKE_LLIR_CREATE_SHARED_MODULE "${LLVM_LINK_BIN} -o <TARGET> <OBJECTS>")
set(CMAKE_LLIR_CREATE_STATIC_LIBRARY "${LLVM_LINK_BIN} -o <TARGET> <OBJECTS>")

add_library(tyr-rt-core ${COMPILE_SOURCES})
set_target_properties(tyr-rt-core
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        LINKER_LANGUAGE LLIR
        OUTPUT_NAME tyr-rt-core.bc
        POSITION_INDEPENDENT_CODE YES
        PREFIX ""
        SUFFIX ""
        RULE_LAUNCH_COMPILE "${CLANG_BIN} <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE> #")
target_compile_options(tyr-rt-core
        PRIVATE
        -flto
        -ffast-math
        -emit-llvm
        -Os)


install(FILES $<TARGET_FILE:tyr-rt-core> DESTINATION lib/tyr)
install(FILES ${COMPILE_HEADERS} DESTINATION include/tyr/rt)
