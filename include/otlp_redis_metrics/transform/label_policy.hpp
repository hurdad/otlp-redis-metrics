#pragma once

#include <string>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

namespace otlp_redis_metrics::transform {

class LabelPolicy {
 public:
  LabelPolicy(std::vector<std::string> allowed_keys, uint32_t max_label_kv, uint32_t max_label_value_len);

  std::vector<std::pair<std::string, std::string>> Canonicalize(
      const std::string& metric_name,
      const std::vector<std::pair<std::string, std::string>>& labels,
      const std::string& service_name_hint,
      const std::string& host_name_hint) const;

 private:
  std::unordered_set<std::string> allowed_keys_;
  uint32_t max_label_kv_;
  uint32_t max_label_value_len_;
};

}  // namespace otlp_redis_metrics::transform
