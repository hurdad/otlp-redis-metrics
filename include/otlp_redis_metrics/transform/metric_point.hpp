#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace otlp_redis_metrics::transform {

struct MetricPoint {
  std::string series_key;
  int64_t timestamp_ms = 0;
  double value = 0.0;
  std::vector<std::pair<std::string, std::string>> labels;
};

}  // namespace otlp_redis_metrics::transform
