#include "otlp_redis_metrics/otlp/metrics_service.hpp"

namespace otlp_redis_metrics::otlp {

MetricsService::MetricsService(runtime::BoundedQueue<transform::MetricPoint>* queue,
                               transform::MetricTransformer* transformer)
    : queue_(queue), transformer_(transformer) {}

grpc::Status MetricsService::Export(
    grpc::ServerContext* /*context*/,
    const opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceRequest* request,
    opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceResponse* response) {
  auto points = transformer_->Transform(*request);
  auto accepted = queue_->TryPushMany(&points);
  if (accepted < points.size()) {
    auto rejected = points.size() - accepted;
    response->mutable_partial_success()->set_rejected_data_points(rejected);
    response->mutable_partial_success()->set_error_message("queue full; dropped points");
  }
  return grpc::Status::OK;
}

}  // namespace otlp_redis_metrics::otlp
