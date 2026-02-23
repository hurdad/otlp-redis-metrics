#pragma once

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "otlp_redis_metrics/config.pb.h"
#include "otlp_redis_metrics/redis/redis_client.hpp"

namespace otlp_redis_metrics::redis {

class TsSchemaManager {
 public:
  TsSchemaManager(RedisClient* client, const ::otlp::redis::metrics::config::ServiceConfig& cfg);

  void EnsureSeries(const std::string& key, const std::vector<std::pair<std::string, std::string>>& labels);

 private:
  bool IsAlreadyExistsError(const std::string& err) const;

  RedisClient* client_;
  const ::otlp::redis::metrics::config::ServiceConfig& cfg_;
  std::unordered_set<std::string> ensured_;
};

}  // namespace otlp_redis_metrics::redis
