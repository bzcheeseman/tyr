add_gtest(StructGen tyr-codegen)
add_gtest(BindGen tyr-codegen)
add_gtest(Parser tyr-frontend)

file(GLOB ALL_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

set(ALL_CXX_SOURCE_FILES ${ALL_TEST_FILES} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)
