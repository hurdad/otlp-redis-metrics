#include "otlp_redis_metrics/transform/label_policy.hpp"

#include <algorithm>

namespace otlp_redis_metrics::transform {

LabelPolicy::LabelPolicy(std::vector<std::string> allowed_keys, uint32_t max_label_kv, uint32_t max_label_value_len)
    : allowed_keys_(allowed_keys.begin(), allowed_keys.end()),
      max_label_kv_(max_label_kv),
      max_label_value_len_(max_label_value_len) {}

std::vector<std::pair<std::string, std::string>> LabelPolicy::Canonicalize(
    const std::string& metric_name, const std::vector<std::pair<std::string, std::string>>& labels,
    const std::string& service_name_hint, const std::string& host_name_hint) const {
  std::vector<std::pair<std::string, std::string>> out;
  out.reserve(labels.size() + 3);
  for (const auto& kv : labels) {
    if (!allowed_keys_.count(kv.first)) {
      continue;
    }
    std::string v = kv.second;
    if (v.size() > max_label_value_len_) {
      v.resize(max_label_value_len_);
    }
    out.push_back({kv.first, std::move(v)});
  }

  if (!host_name_hint.empty() && allowed_keys_.count("host")) {
    out.push_back({"host", host_name_hint.substr(0, max_label_value_len_)});
  }
  if (!service_name_hint.empty() && allowed_keys_.count("service")) {
    out.push_back({"service", service_name_hint.substr(0, max_label_value_len_)});
  }

  out.push_back({"metric", metric_name.substr(0, max_label_value_len_)});
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  if (out.size() > max_label_kv_) {
    out.resize(max_label_kv_);
  }
  return out;
}

}  // namespace otlp_redis_metrics::transform
