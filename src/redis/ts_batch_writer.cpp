#include "otlp_redis_metrics/redis/ts_batch_writer.hpp"

#include <chrono>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace otlp_redis_metrics::redis {

TsBatchWriter::TsBatchWriter(runtime::BoundedQueue<transform::MetricPoint>* queue, TsSchemaManager* schema, RedisClient* redis,
                             const ::otlp::redis::metrics::config::ServiceConfig& cfg)
    : queue_(queue), schema_(schema), redis_(redis), cfg_(cfg) {}

void TsBatchWriter::Start() {
  stop_.store(false);
  thread_ = std::thread([this] { Run(); });
}

void TsBatchWriter::Stop() {
  stop_.store(true);
  if (thread_.joinable()) {
    thread_.join();
  }
}

void TsBatchWriter::Run() {
  std::vector<transform::MetricPoint> buf;
  auto last_flush = std::chrono::steady_clock::now();

  while (!stop_.load()) {
    transform::MetricPoint item;
    if (queue_->PopWait(&item, cfg_.ingest().flush_interval_ms())) {
      buf.push_back(std::move(item));
      queue_->DrainSome(&buf, cfg_.ingest().max_batch_points() - buf.size());
    }

    auto now = std::chrono::steady_clock::now();
    bool time_due = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush).count() >=
                    cfg_.ingest().flush_interval_ms();
    if (buf.empty()) continue;
    if (buf.size() >= cfg_.ingest().max_batch_points() || time_due) {
      Flush(buf);
      buf.clear();
      last_flush = now;
    }
  }

  if (!buf.empty()) {
    Flush(buf);
  }
}

void TsBatchWriter::Flush(const std::vector<transform::MetricPoint>& points) {
  std::unordered_set<std::string> ensured;
  for (const auto& p : points) {
    if (ensured.insert(p.series_key).second) {
      schema_->EnsureSeries(p.series_key, p.labels);
    }
  }

  size_t i = 0;
  while (i < points.size()) {
    size_t end = std::min(points.size(), i + static_cast<size_t>(cfg_.ingest().max_batch_points()));
    std::ostringstream cmd;
    cmd << "TS.MADD";
    for (size_t j = i; j < end; ++j) {
      cmd << ' ' << points[j].series_key << ' ' << points[j].timestamp_ms << ' ' << points[j].value;
    }
    auto res = redis_->Command(cmd.str());
    if (!res.has_value() || res->find("ERR") != std::string::npos) {
      std::cerr << "redis batch write failed\n";
    }
    i = end;
  }
}

}  // namespace otlp_redis_metrics::redis
