#pragma once

#include <hiredis/hiredis.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "otlp_redis_metrics/config.pb.h"

namespace otlp_redis_metrics::redis {

class RedisClient {
 public:
  RedisClient();
  ~RedisClient();

  bool Connect(const ::otlp::redis::metrics::config::RedisConfig& cfg);
  std::optional<std::string> Command(const std::string& cmd);
  bool Pipeline(const std::vector<std::string>& cmds);

 private:
  redisContext* ctx_;
};

}  // namespace otlp_redis_metrics::redis
