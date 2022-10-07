cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(
    PROTO_SRCS
    PROTO_HDRS
    protos/transport_catalog.proto
)

add_executable(main ${PROTO_SRCS} ${PROTO_HDRS} project/canvas.cpp  project/svg.cpp  project/json.cpp  project/main.cpp  project/sphere.cpp  project/transport_catalog.cpp project/serialize.cpp project/router.cpp)
target_link_libraries(main ${Protobuf_LIBRARIES})