#include "otlp_redis_metrics/redis/ts_schema_manager.hpp"

#include <sstream>

namespace otlp_redis_metrics::redis {

TsSchemaManager::TsSchemaManager(RedisClient* client, const ::otlp::redis::metrics::config::ServiceConfig& cfg)
    : client_(client), cfg_(cfg) {}

bool TsSchemaManager::IsAlreadyExistsError(const std::string& err) const {
  return err.find("already exists") != std::string::npos || err.find("exists") != std::string::npos;
}

void TsSchemaManager::EnsureSeries(const std::string& key,
                                   const std::vector<std::pair<std::string, std::string>>& labels) {
  if (ensured_.count(key)) return;

  std::ostringstream cmd;
  cmd << "TS.CREATE " << key << " RETENTION " << cfg_.timeseries().retention_ms() << " LABELS";
  for (const auto& kv : labels) {
    cmd << ' ' << kv.first << ' ' << kv.second;
  }

  auto res = client_->Command(cmd.str());
  if (res.has_value() && !IsAlreadyExistsError(*res)) {
    ensured_.insert(key);
  }
  if (!cfg_.timeseries().ensure_rules()) {
    return;
  }

  for (const auto window_ms : cfg_.timeseries().downsample_ms()) {
    std::string target = key + ":avg_" + std::to_string(window_ms) + "ms";
    std::ostringstream tcmd;
    tcmd << "TS.CREATE " << target << " RETENTION " << cfg_.timeseries().retention_ms() << " LABELS";
    for (const auto& kv : labels) {
      tcmd << ' ' << kv.first << ' ' << kv.second;
    }
    tcmd << " rollup avg_" << window_ms << "ms";
    client_->Command(tcmd.str());

    std::ostringstream rcmd;
    rcmd << "TS.CREATERULE " << key << ' ' << target << " AGGREGATION avg " << window_ms;
    client_->Command(rcmd.str());
  }

  ensured_.insert(key);
}

}  // namespace otlp_redis_metrics::redis
