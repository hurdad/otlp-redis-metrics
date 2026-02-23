#pragma once

#include <memory>

#include <grpcpp/server.h>

#include "otlp_redis_metrics/config.pb.h"
#include "otlp_redis_metrics/otlp/metrics_service.hpp"

namespace otlp_redis_metrics::otlp {

class OtlpServer {
 public:
  OtlpServer(const ::otlp::redis::metrics::config::ServiceConfig& cfg, MetricsService* service);

  bool Start();
  void Stop();

 private:
  const ::otlp::redis::metrics::config::ServiceConfig& cfg_;
  MetricsService* service_;
  std::unique_ptr<grpc::Server> server_;
};

}  // namespace otlp_redis_metrics::otlp
