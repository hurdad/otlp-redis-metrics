#include "otlp_redis_metrics/config/config_loader.hpp"

#include <fstream>
#include <sstream>

#include <google/protobuf/util/json_util.h>

namespace otlp_redis_metrics::config {

std::optional<::otlp::redis::metrics::config::ServiceConfig> ConfigLoader::Load(const std::string& config_json,
                                                                                const std::string& config_bin,
                                                                                std::string* err) {
  ::otlp::redis::metrics::config::ServiceConfig cfg;
  if (!config_json.empty()) {
    std::ifstream in(config_json);
    if (!in) {
      *err = "failed opening JSON config: " + config_json;
      return std::nullopt;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    auto status = google::protobuf::util::JsonStringToMessage(ss.str(), &cfg);
    if (!status.ok()) {
      *err = "failed parsing JSON config: " + std::string(status.message());
      return std::nullopt;
    }
  } else if (!config_bin.empty()) {
    std::ifstream in(config_bin, std::ios::binary);
    if (!in) {
      *err = "failed opening binary config: " + config_bin;
      return std::nullopt;
    }
    if (!cfg.ParseFromIstream(&in)) {
      *err = "failed parsing binary protobuf config";
      return std::nullopt;
    }
  }

  ApplyDefaults(&cfg);
  return cfg;
}

void ConfigLoader::ApplyDefaults(::otlp::redis::metrics::config::ServiceConfig* cfg) {
  if (cfg->redis().unix_socket().empty()) {
    if (cfg->redis().host().empty()) cfg->mutable_redis()->set_host("127.0.0.1");
    if (cfg->redis().port() == 0) cfg->mutable_redis()->set_port(6379);
  }
  if (cfg->timeseries().key_prefix().empty()) cfg->mutable_timeseries()->set_key_prefix("metrics:");
  if (cfg->timeseries().retention_ms() == 0) cfg->mutable_timeseries()->set_retention_ms(86400000);
  if (!cfg->timeseries().has_create_on_write()) cfg->mutable_timeseries()->set_create_on_write(true);
  if (!cfg->timeseries().has_ensure_rules()) cfg->mutable_timeseries()->set_ensure_rules(true);
  if (cfg->timeseries().downsample_ms_size() == 0) {
    cfg->mutable_timeseries()->add_downsample_ms(60000);
    cfg->mutable_timeseries()->add_downsample_ms(300000);
  }

  if (cfg->ingest().max_queue_depth() == 0) cfg->mutable_ingest()->set_max_queue_depth(50000);
  if (cfg->ingest().max_batch_points() == 0) cfg->mutable_ingest()->set_max_batch_points(5000);
  if (cfg->ingest().flush_interval_ms() == 0) cfg->mutable_ingest()->set_flush_interval_ms(200);
  if (cfg->ingest().max_label_kv() == 0) cfg->mutable_ingest()->set_max_label_kv(12);
  if (cfg->ingest().max_label_value_len() == 0) cfg->mutable_ingest()->set_max_label_value_len(128);

  if (cfg->otlp().listen_addr().empty()) cfg->mutable_otlp()->set_listen_addr("0.0.0.0:4317");
  if (cfg->otlp().grpc_threads() == 0) cfg->mutable_otlp()->set_grpc_threads(2);

  if (cfg->label_policy().allowed_keys_size() == 0) {
    for (const auto& k : {"host", "service", "instance", "core", "gpu", "queue", "device"}) {
      cfg->mutable_label_policy()->add_allowed_keys(k);
    }
  }
}

}  // namespace otlp_redis_metrics::config
