set(ALL_CXX_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_executable(tyr ${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp)

find_package(Threads)
target_link_libraries(tyr tyr-codegen tyr-frontend Threads::Threads)

add_dependencies(tyr tyr-rt-core)

if (CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-DTYR_RT_BITCODE_DIR="${CMAKE_INSTALL_PREFIX}/lib/tyr/")
else()
    add_definitions(-DTYR_RT_BITCODE_DIR="${CMAKE_BINARY_DIR}/")
endif()

install(EXPORT tyr-targets
        FILE tyr-targets.cmake
        NAMESPACE tyrc::
        DESTINATION lib/cmake/tyr
        )

configure_file(${CMAKE_SOURCE_DIR}/cmake/tyr-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/tyr-config.cmake @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/cmake/tyr-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/tyr-config-version.cmake @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tyr-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/tyr-config-version.cmake"
        DESTINATION lib/cmake/tyr
        )

install(TARGETS tyr EXPORT tyr-targets
        RUNTIME DESTINATION bin)
