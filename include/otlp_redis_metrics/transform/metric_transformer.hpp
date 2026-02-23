#pragma once

#include <vector>

#include "opentelemetry/proto/collector/metrics/v1/metrics_service.pb.h"
#include "otlp_redis_metrics/transform/label_policy.hpp"
#include "otlp_redis_metrics/transform/metric_point.hpp"
#include "otlp_redis_metrics/transform/series_keyer.hpp"

namespace otlp_redis_metrics::transform {

class MetricTransformer {
 public:
  MetricTransformer(LabelPolicy label_policy, SeriesKeyer series_keyer, bool normalize_metric_names);

  std::vector<MetricPoint> Transform(
      const opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceRequest& req) const;

 private:
  std::string NormalizeMetricName(const std::string& name) const;
  void AddPoint(std::vector<MetricPoint>* out, const std::string& metric_name, int64_t ts_ns, double value,
                const std::vector<std::pair<std::string, std::string>>& attrs, const std::string& service_hint,
                const std::string& host_hint) const;

  LabelPolicy label_policy_;
  SeriesKeyer series_keyer_;
  bool normalize_metric_names_;
};

}  // namespace otlp_redis_metrics::transform
