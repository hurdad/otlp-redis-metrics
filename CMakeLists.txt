cmake_minimum_required(VERSION 3.24)
project(otlp-redis-metrics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)

include(FetchContent)
FetchContent_Declare(
  opentelemetry_proto
  GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-proto.git
  GIT_TAG v1.3.2
)
FetchContent_MakeAvailable(opentelemetry_proto)

set(PROTO_IMPORT_DIRS
  -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
  -I ${opentelemetry_proto_SOURCE_DIR}
  -I ${Protobuf_INCLUDE_DIRS}
)

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

set(PROTO_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/proto/otlp_redis_metrics/config.proto
  ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/common/v1/common.proto
  ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/resource/v1/resource.proto
  ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/metrics/v1/metrics.proto
  ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/collector/metrics/v1/metrics_service.proto
)

set(GEN_SRCS
  ${GEN_DIR}/otlp_redis_metrics/config.pb.cc
  ${GEN_DIR}/opentelemetry/proto/common/v1/common.pb.cc
  ${GEN_DIR}/opentelemetry/proto/resource/v1/resource.pb.cc
  ${GEN_DIR}/opentelemetry/proto/metrics/v1/metrics.pb.cc
  ${GEN_DIR}/opentelemetry/proto/collector/metrics/v1/metrics_service.pb.cc
  ${GEN_DIR}/opentelemetry/proto/collector/metrics/v1/metrics_service.grpc.pb.cc
)

add_custom_command(
  OUTPUT ${GEN_SRCS}
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
  ARGS ${PROTO_IMPORT_DIRS}
       --cpp_out=${GEN_DIR}
       --grpc_out=${GEN_DIR}
       --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
       ${PROTO_FILES}
  DEPENDS ${PROTO_FILES}
  VERBATIM
)

add_library(otlp_proto ${GEN_SRCS})
target_include_directories(otlp_proto PUBLIC ${GEN_DIR})
target_link_libraries(otlp_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)

add_executable(otlp-redis-metrics
  src/main.cpp
  src/app.cpp
  src/config/config_loader.cpp
  src/otlp/otlp_server.cpp
  src/otlp/metrics_service.cpp
  src/transform/label_policy.cpp
  src/transform/series_keyer.cpp
  src/transform/metric_transformer.cpp
  src/redis/redis_client.cpp
  src/redis/ts_schema_manager.cpp
  src/redis/ts_batch_writer.cpp
)

target_include_directories(otlp-redis-metrics PRIVATE include ${GEN_DIR})
target_link_libraries(otlp-redis-metrics PRIVATE otlp_proto protobuf::libprotobuf gRPC::grpc++ PkgConfig::HIREDIS)
