#include "otlp_redis_metrics/redis/ts_schema_manager.hpp"

namespace otlp_redis_metrics::redis {

TsSchemaManager::TsSchemaManager(RedisClient* client, const ::otlp::redis::metrics::config::ServiceConfig& cfg)
    : client_(client), cfg_(cfg) {}

bool TsSchemaManager::IsAlreadyExistsError(const std::string& err) const {
  return err.find("already exists") != std::string::npos || err.find("exists") != std::string::npos;
}

void TsSchemaManager::EnsureSeries(const std::string& key,
                                   const std::vector<std::pair<std::string, std::string>>& labels) {
  if (ensured_.count(key)) return;

  std::vector<std::string> create_args = {"TS.CREATE", key, "RETENTION", std::to_string(cfg_.timeseries().retention_ms()),
                                          "LABELS"};
  create_args.reserve(create_args.size() + (labels.size() * 2));
  for (const auto& kv : labels) {
    create_args.push_back(kv.first);
    create_args.push_back(kv.second);
  }

  auto res = client_->CommandArgv(create_args);
  if (res.has_value() && !IsAlreadyExistsError(*res)) {
    ensured_.insert(key);
  }
  if (!cfg_.timeseries().ensure_rules()) {
    return;
  }

  for (const auto window_ms : cfg_.timeseries().downsample_ms()) {
    std::string target = key + ":avg_" + std::to_string(window_ms) + "ms";
    std::vector<std::string> target_create_args = {
        "TS.CREATE", target, "RETENTION", std::to_string(cfg_.timeseries().retention_ms()), "LABELS"};
    target_create_args.reserve(target_create_args.size() + (labels.size() * 2) + 2);
    for (const auto& kv : labels) {
      target_create_args.push_back(kv.first);
      target_create_args.push_back(kv.second);
    }
    target_create_args.push_back("rollup");
    target_create_args.push_back("avg_" + std::to_string(window_ms) + "ms");
    client_->CommandArgv(target_create_args);

    client_->CommandArgv(
        {"TS.CREATERULE", key, target, "AGGREGATION", "avg", std::to_string(window_ms)});
  }

  ensured_.insert(key);
}

}  // namespace otlp_redis_metrics::redis
