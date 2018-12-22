set(ALL_CXX_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_executable(tyr ${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp)

find_package(Threads)
target_link_libraries(tyr tyr-codegen tyr-frontend Threads::Threads)

add_dependencies(tyr tyr-rt-core)

if (CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-DTYR_RT_BITCODE_FILE="${CMAKE_INSTALL_PREFIX}/lib/tyr/tyr-rt-core.bc")
else()
    add_definitions(-DTYR_RT_BITCODE_FILE="${CMAKE_BINARY_DIR}/tyr-rt-core.bc")
endif()

install(TARGETS tyr EXPORT tyrc
        CONFIGURATIONS Release
        RUNTIME DESTINATION bin)
