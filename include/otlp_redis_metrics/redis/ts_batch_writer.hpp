#pragma once

#include <thread>

#include "otlp_redis_metrics/config.pb.h"
#include "otlp_redis_metrics/redis/ts_schema_manager.hpp"
#include "otlp_redis_metrics/runtime/bounded_queue.hpp"
#include "otlp_redis_metrics/transform/metric_point.hpp"

namespace otlp_redis_metrics::redis {

class TsBatchWriter {
 public:
  TsBatchWriter(runtime::BoundedQueue<transform::MetricPoint>* queue, TsSchemaManager* schema, RedisClient* redis,
                const ::otlp::redis::metrics::config::ServiceConfig& cfg);

  void Start();
  void Stop();

 private:
  void Run();
  void Flush(const std::vector<transform::MetricPoint>& points);

  runtime::BoundedQueue<transform::MetricPoint>* queue_;
  TsSchemaManager* schema_;
  RedisClient* redis_;
  const ::otlp::redis::metrics::config::ServiceConfig& cfg_;
  std::thread thread_;
  bool stop_{false};
};

}  // namespace otlp_redis_metrics::redis
