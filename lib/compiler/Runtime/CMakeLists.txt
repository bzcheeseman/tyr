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
        llvm-link
        llvm-ld)

set(CMAKE_LLIR_CREATE_SHARED_LIBRARY "${LLVM_LINK_BIN} -o <TARGET> <OBJECTS>")
set(CMAKE_LLIR_CREATE_SHARED_MODULE "${LLVM_LINK_BIN} -o <TARGET> <OBJECTS>")
set(CMAKE_LLIR_CREATE_STATIC_LIBRARY "${LLVM_LINK_BIN} -o <TARGET> <OBJECTS>")

add_custom_target(tyr-rt-core)

function(add_tyr_rt_bc SUFFIX SOURCE HEADER)
    add_library(tyr-rt-${SUFFIX} ${SOURCE} ${HEADER})
    set_target_properties(tyr-rt-${SUFFIX}
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
            C_STANDARD 11
            C_STANDARD_REQUIRED YES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
            LINKER_LANGUAGE LLIR
            OUTPUT_NAME tyr-rt-${SUFFIX}.bc
            POSITION_INDEPENDENT_CODE YES
            PREFIX ""
            SUFFIX ""
            RULE_LAUNCH_COMPILE "${CLANG_BIN} <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE> #")
    target_compile_options(tyr-rt-${SUFFIX}
            PRIVATE
            -g0
            -mtune=generic
            -march=x86-64
            -flto
            -ffast-math
            -emit-llvm
            -Oz)

    add_dependencies(tyr-rt-core tyr-rt-${SUFFIX})
    install(FILES $<TARGET_FILE:tyr-rt-${SUFFIX}> DESTINATION lib/tyr)
    install(FILES ${HEADER} DESTINATION include/tyr/rt)
endfunction()

add_tyr_rt_bc(file FileHelper.c FileHelper.h)
add_tyr_rt_bc(base64 Base64.c Base64.h)

