cmake_minimum_required(VERSION 3.10)
project(tyr-rt)

find_package(Protobuf 3.5 REQUIRED)

file(GLOB proto_files ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
protobuf_generate_cpp(PROTO_SRC PROTO_HDR ${proto_files})

file(GLOB RT_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)
file(GLOB RT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

llvm_map_components_to_libnames(LLVM_LIBS all)

set(ALL_CXX_SOURCE_FILES ${RT_INCLUDE} ${RT_SOURCES} ${ALL_CXX_SOURCE_FILES} PARENT_SCOPE)

add_library(tyr-rt SHARED ${RT_SOURCES} ${RT_INCLUDE} ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR})
target_link_libraries(tyr-rt PUBLIC ${PROTOBUF_LIBRARIES} ${LLVM_LIBS})
target_include_directories(tyr-rt
        PUBLIC
            $<INSTALL_INTERFACE:include/rt>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE ${LLVM_INCLUDE_DIRS}
        PRIVATE ${PROTOBUF_INCLUDE_DIRS}
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)
