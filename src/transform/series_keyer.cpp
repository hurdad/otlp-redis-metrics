#include "otlp_redis_metrics/transform/series_keyer.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>

namespace otlp_redis_metrics::transform {

SeriesKeyer::SeriesKeyer(std::string key_prefix) : key_prefix_(std::move(key_prefix)) {}

uint64_t SeriesKeyer::Fnv1a64(const std::string& input) {
  uint64_t hash = 1469598103934665603ULL;
  for (unsigned char c : input) {
    hash ^= static_cast<uint64_t>(c);
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::string SeriesKeyer::MakeSeriesKey(const std::string& metric_name,
                                       const std::vector<std::pair<std::string, std::string>>& canonical_labels) const {
  std::string canonical;
  canonical.reserve(canonical_labels.size() * 24);
  for (const auto& kv : canonical_labels) {
    canonical.append(kv.first);
    canonical.push_back('=');
    canonical.append(kv.second);
    canonical.push_back(';');
  }

  std::ostringstream os;
  os << key_prefix_ << metric_name << ':' << std::hex << std::setw(16) << std::setfill('0') << Fnv1a64(canonical);
  return os.str();
}

}  // namespace otlp_redis_metrics::transform
