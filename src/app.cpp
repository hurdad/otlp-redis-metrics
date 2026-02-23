#include "otlp_redis_metrics/app.hpp"

#include <csignal>
#include <ctime>
#include <iostream>
#include <sstream>
#include <thread>

#include <google/protobuf/util/json_util.h>

#include "otlp_redis_metrics/config/config_loader.hpp"
#include "otlp_redis_metrics/otlp/metrics_service.hpp"
#include "otlp_redis_metrics/otlp/otlp_server.hpp"
#include "otlp_redis_metrics/redis/redis_client.hpp"
#include "otlp_redis_metrics/redis/ts_batch_writer.hpp"
#include "otlp_redis_metrics/redis/ts_schema_manager.hpp"
#include "otlp_redis_metrics/runtime/bounded_queue.hpp"
#include "otlp_redis_metrics/runtime/shutdown.hpp"
#include "otlp_redis_metrics/transform/label_policy.hpp"
#include "otlp_redis_metrics/transform/metric_point.hpp"
#include "otlp_redis_metrics/transform/metric_transformer.hpp"
#include "otlp_redis_metrics/transform/series_keyer.hpp"

namespace {
otlp_redis_metrics::runtime::ShutdownSignal* g_shutdown = nullptr;

void HandleSignal(int) {
  if (g_shutdown != nullptr) {
    g_shutdown->Request();
  }
}

std::string ArgValue(int argc, char** argv, const std::string& key) {
  const std::string prefix = "--" + key + "=";
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a.rfind(prefix, 0) == 0) return a.substr(prefix.size());
  }
  return "";
}

bool ArgBool(int argc, char** argv, const std::string& key) { return ArgValue(argc, argv, key) == "true"; }
}  // namespace

namespace otlp_redis_metrics {

void LogInfo(const std::string& msg) {
  std::time_t t = std::time(nullptr);
  std::cerr << "[INFO " << std::asctime(std::localtime(&t)) << "] " << msg << '\n';
}

void LogError(const std::string& msg) {
  std::time_t t = std::time(nullptr);
  std::cerr << "[ERROR " << std::asctime(std::localtime(&t)) << "] " << msg << '\n';
}

int Run(int argc, char** argv) {
  std::string err;
  auto cfg_opt = config::ConfigLoader::Load(ArgValue(argc, argv, "config_json"), ArgValue(argc, argv, "config_bin"), &err);
  if (!cfg_opt.has_value()) {
    LogError(err);
    return 1;
  }
  auto cfg = *cfg_opt;

  if (ArgBool(argc, argv, "print_effective_config")) {
    std::string json;
    google::protobuf::util::MessageToJsonString(cfg, &json);
    std::cout << json << '\n';
    return 0;
  }

  redis::RedisClient redis;
  if (!redis.Connect(cfg.redis())) {
    LogError("failed to connect to redis");
    return 1;
  }

  if (ArgBool(argc, argv, "self_test")) {
    auto create = redis.Command("TS.CREATE self_test_metric");
    auto add = redis.Command("TS.ADD self_test_metric * 1");
    auto info = redis.Command("TS.INFO self_test_metric");
    if (!create.has_value() || !add.has_value() || !info.has_value()) {
      LogError("self_test failed");
      return 1;
    }
    LogInfo("self_test passed");
    return 0;
  }

  runtime::BoundedQueue<transform::MetricPoint> queue(cfg.ingest().max_queue_depth());
  transform::LabelPolicy label_policy(
      std::vector<std::string>(cfg.label_policy().allowed_keys().begin(), cfg.label_policy().allowed_keys().end()),
      cfg.ingest().max_label_kv(), cfg.ingest().max_label_value_len());
  transform::SeriesKeyer keyer(cfg.timeseries().key_prefix());
  transform::MetricTransformer transformer(label_policy, keyer, cfg.normalize_metric_names());

  redis::TsSchemaManager schema(&redis, cfg);
  redis::TsBatchWriter writer(&queue, &schema, &redis, cfg);
  writer.Start();

  otlp::MetricsService service(&queue, &transformer);
  otlp::OtlpServer server(cfg, &service);
  if (!server.Start()) {
    LogError("failed to start OTLP gRPC server");
    queue.Shutdown();
    writer.Stop();
    return 1;
  }

  runtime::ShutdownSignal shutdown;
  g_shutdown = &shutdown;
  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  LogInfo("service started on " + cfg.otlp().listen_addr());
  while (!shutdown.Requested()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  server.Stop();
  queue.Shutdown();
  writer.Stop();
  LogInfo("service stopped");
  return 0;
}

}  // namespace otlp_redis_metrics
