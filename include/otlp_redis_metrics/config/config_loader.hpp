#pragma once

#include <optional>
#include <string>

#include "otlp_redis_metrics/config.pb.h"

namespace otlp_redis_metrics::config {

class ConfigLoader {
 public:
  static std::optional<::otlp::redis::metrics::config::ServiceConfig> Load(const std::string& config_json,
                                                                          const std::string& config_bin,
                                                                          std::string* err);
  static void ApplyDefaults(::otlp::redis::metrics::config::ServiceConfig* cfg);
};

}  // namespace otlp_redis_metrics::config
