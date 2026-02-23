#include "otlp_redis_metrics/transform/metric_transformer.hpp"

namespace otlp_redis_metrics::transform {

MetricTransformer::MetricTransformer(LabelPolicy label_policy, SeriesKeyer series_keyer, bool normalize_metric_names)
    : label_policy_(std::move(label_policy)),
      series_keyer_(std::move(series_keyer)),
      normalize_metric_names_(normalize_metric_names) {}

std::string MetricTransformer::NormalizeMetricName(const std::string& name) const {
  if (!normalize_metric_names_) return name;
  std::string out = name;
  for (char& c : out) {
    if (c == '.' || c == ' ' || c == '/') c = '_';
  }
  return out;
}

void MetricTransformer::AddPoint(std::vector<MetricPoint>* out, const std::string& metric_name, int64_t ts_ns, double value,
                                 const std::vector<std::pair<std::string, std::string>>& attrs,
                                 const std::string& service_hint, const std::string& host_hint) const {
  auto normalized_name = NormalizeMetricName(metric_name);
  auto labels = label_policy_.Canonicalize(normalized_name, attrs, service_hint, host_hint);
  MetricPoint pt;
  pt.labels = labels;
  pt.series_key = series_keyer_.MakeSeriesKey(normalized_name, labels);
  pt.timestamp_ms = ts_ns / 1000000;
  pt.value = value;
  out->push_back(std::move(pt));
}

std::vector<MetricPoint> MetricTransformer::Transform(
    const opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceRequest& req) const {
  std::vector<MetricPoint> out;
  for (const auto& rm : req.resource_metrics()) {
    std::string host_hint;
    for (const auto& a : rm.resource().attributes()) {
      if (a.key() == "host.name" && a.value().has_string_value()) {
        host_hint = a.value().string_value();
      }
    }

    for (const auto& sm : rm.scope_metrics()) {
      std::string service_hint = sm.scope().name();
      for (const auto& m : sm.metrics()) {
        std::vector<std::pair<std::string, std::string>> attrs;
        if (m.has_gauge()) {
          for (const auto& dp : m.gauge().data_points()) {
            attrs.clear();
            for (const auto& a : dp.attributes()) {
              if (a.value().has_string_value()) attrs.push_back({a.key(), a.value().string_value()});
            }
            AddPoint(&out, m.name(), dp.time_unix_nano(), dp.as_double(), attrs, service_hint, host_hint);
          }
        } else if (m.has_sum()) {
          for (const auto& dp : m.sum().data_points()) {
            attrs.clear();
            for (const auto& a : dp.attributes()) {
              if (a.value().has_string_value()) attrs.push_back({a.key(), a.value().string_value()});
            }
            AddPoint(&out, m.name(), dp.time_unix_nano(), dp.as_double(), attrs, service_hint, host_hint);
          }
        } else if (m.has_histogram()) {
          for (const auto& dp : m.histogram().data_points()) {
            attrs.clear();
            for (const auto& a : dp.attributes()) {
              if (a.value().has_string_value()) attrs.push_back({a.key(), a.value().string_value()});
            }
            AddPoint(&out, m.name() + "_count", dp.time_unix_nano(), static_cast<double>(dp.count()), attrs, service_hint,
                     host_hint);
            AddPoint(&out, m.name() + "_sum", dp.time_unix_nano(), dp.sum(), attrs, service_hint, host_hint);
            if (dp.count() > 0) {
              AddPoint(&out, m.name() + "_avg", dp.time_unix_nano(), dp.sum() / static_cast<double>(dp.count()), attrs,
                       service_hint, host_hint);
            }
          }
        }
      }
    }
  }
  return out;
}

}  // namespace otlp_redis_metrics::transform
