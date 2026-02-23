#pragma once

#include <string>
#include <cstdint>
#include <utility>
#include <vector>

namespace otlp_redis_metrics::transform {

class SeriesKeyer {
 public:
  explicit SeriesKeyer(std::string key_prefix);

  std::string MakeSeriesKey(const std::string& metric_name,
                            const std::vector<std::pair<std::string, std::string>>& canonical_labels) const;

 private:
  static uint64_t Fnv1a64(const std::string& input);

  std::string key_prefix_;
};

}  // namespace otlp_redis_metrics::transform
