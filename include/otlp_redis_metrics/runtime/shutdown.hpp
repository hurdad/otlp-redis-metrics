#pragma once

#include <atomic>

namespace otlp_redis_metrics::runtime {

class ShutdownSignal {
 public:
  void Request() { requested_.store(true); }
  bool Requested() const { return requested_.load(); }

 private:
  std::atomic<bool> requested_{false};
};

}  // namespace otlp_redis_metrics::runtime
