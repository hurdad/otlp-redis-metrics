#include <algorithm>

#include "otlp_redis_metrics/otlp/otlp_server.hpp"

#include <grpcpp/server_builder.h>

namespace otlp_redis_metrics::otlp {

OtlpServer::OtlpServer(const ::otlp::redis::metrics::config::ServiceConfig& cfg, MetricsService* service)
    : cfg_(cfg), service_(service) {}

bool OtlpServer::Start() {
  grpc::ServerBuilder builder;
  const int grpc_threads = std::max(1, static_cast<int>(cfg_.otlp().grpc_threads()));
  builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, grpc_threads);
  builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, grpc_threads);
  builder.AddListeningPort(cfg_.otlp().listen_addr(), grpc::InsecureServerCredentials());
  builder.RegisterService(service_);
  server_ = builder.BuildAndStart();
  return static_cast<bool>(server_);
}

void OtlpServer::Stop() {
  if (server_) {
    server_->Shutdown();
  }
}

}  // namespace otlp_redis_metrics::otlp
