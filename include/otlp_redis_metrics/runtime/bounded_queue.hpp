#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

namespace otlp_redis_metrics::runtime {

template <typename T>
class BoundedQueue {
 public:
  explicit BoundedQueue(size_t capacity) : capacity_(capacity) {}

  bool TryPush(T value) {
    std::lock_guard<std::mutex> lock(mu_);
    if (shutdown_ || q_.size() >= capacity_) {
      return false;
    }
    q_.push_back(std::move(value));
    cv_.notify_one();
    return true;
  }

  size_t TryPushMany(std::vector<T>* values) {
    std::lock_guard<std::mutex> lock(mu_);
    if (shutdown_) {
      return 0;
    }
    size_t accepted = 0;
    for (auto& v : *values) {
      if (q_.size() >= capacity_) {
        break;
      }
      q_.push_back(std::move(v));
      ++accepted;
    }
    if (accepted > 0) {
      cv_.notify_one();
    }
    return accepted;
  }

  bool PopWait(T* out, uint32_t timeout_ms) {
    std::unique_lock<std::mutex> lock(mu_);
    cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return shutdown_ || !q_.empty(); });
    if (q_.empty()) {
      return false;
    }
    *out = std::move(q_.front());
    q_.pop_front();
    return true;
  }

  size_t DrainSome(std::vector<T>* out, size_t max_items) {
    std::lock_guard<std::mutex> lock(mu_);
    size_t n = std::min(max_items, q_.size());
    out->reserve(out->size() + n);
    for (size_t i = 0; i < n; ++i) {
      out->push_back(std::move(q_.front()));
      q_.pop_front();
    }
    return n;
  }

  void Shutdown() {
    std::lock_guard<std::mutex> lock(mu_);
    shutdown_ = true;
    cv_.notify_all();
  }

  bool IsShutdown() const {
    std::lock_guard<std::mutex> lock(mu_);
    return shutdown_;
  }

 private:
  const size_t capacity_;
  mutable std::mutex mu_;
  std::condition_variable cv_;
  std::deque<T> q_;
  bool shutdown_{false};
};

}  // namespace otlp_redis_metrics::runtime
