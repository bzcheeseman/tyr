if (NOT __ADD_CLANG_FORMAT_INCLUDED)
    set(__ADD_CLANG_FORMAT_INCLUDED TRUE)

    find_program(CLANG_FORMAT "clang-format")
    if(CLANG_FORMAT)
        add_custom_target(
                clang-format
                COMMAND ${CLANG_FORMAT}
                -i
                -style=file
                ${ALL_CXX_SOURCE_FILES}

        )
    endif (CLANG_FORMAT)
endif (NOT __ADD_CLANG_FORMAT_INCLUDED)