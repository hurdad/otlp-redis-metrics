#pragma once

#include "opentelemetry/proto/collector/metrics/v1/metrics_service.grpc.pb.h"
#include "otlp_redis_metrics/runtime/bounded_queue.hpp"
#include "otlp_redis_metrics/transform/metric_point.hpp"
#include "otlp_redis_metrics/transform/metric_transformer.hpp"

namespace otlp_redis_metrics::otlp {

class MetricsService final : public opentelemetry::proto::collector::metrics::v1::MetricsService::Service {
 public:
  MetricsService(runtime::BoundedQueue<transform::MetricPoint>* queue, transform::MetricTransformer* transformer);

  grpc::Status Export(grpc::ServerContext* context,
                      const opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceRequest* request,
                      opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceResponse* response) override;

 private:
  runtime::BoundedQueue<transform::MetricPoint>* queue_;
  transform::MetricTransformer* transformer_;
};

}  // namespace otlp_redis_metrics::otlp
