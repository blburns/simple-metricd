/**
 * @file metric.cpp
 */

#include "simple-metricd/metric/metric.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <mutex>

namespace simple_metricd {

namespace {

std::string lowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

class CounterMetric : public Metric {
public:
  explicit CounterMetric(MetricSpec spec)
      : name_(std::move(spec.name)), help_(std::move(spec.help)),
        labels_(std::move(spec.labels)),
        value_(spec.value < 0.0 ? 0.0 : spec.value) {}

  std::string name() const override { return name_; }
  MetricType type() const override { return MetricType::Counter; }
  double value() const override {
    std::lock_guard<std::mutex> lock(mutex_);
    return value_;
  }
  bool setValue(double value) override {
    if (value < 0.0 || !std::isfinite(value)) {
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (value < value_) {
      return false;
    }
    value_ = value;
    return true;
  }
  std::string help() const override { return help_; }
  std::string labels() const override { return labels_; }

private:
  std::string name_;
  std::string help_;
  std::string labels_;
  mutable std::mutex mutex_;
  double value_{0.0};
};

class GaugeMetric : public Metric {
public:
  explicit GaugeMetric(MetricSpec spec)
      : name_(std::move(spec.name)), help_(std::move(spec.help)),
        labels_(std::move(spec.labels)), value_(spec.value) {}

  std::string name() const override { return name_; }
  MetricType type() const override { return MetricType::Gauge; }
  double value() const override {
    std::lock_guard<std::mutex> lock(mutex_);
    return value_;
  }
  bool setValue(double value) override {
    if (!std::isfinite(value)) {
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = value;
    return true;
  }
  std::string help() const override { return help_; }
  std::string labels() const override { return labels_; }

private:
  std::string name_;
  std::string help_;
  std::string labels_;
  mutable std::mutex mutex_;
  double value_{0.0};
};

}  // namespace

MetricType parseMetricType(const std::string &name) {
  const std::string lower = lowerCopy(name);
  if (lower == "counter") return MetricType::Counter;
  if (lower == "gauge") return MetricType::Gauge;
  return MetricType::Unknown;
}

const char *toString(MetricType type) {
  switch (type) {
  case MetricType::Counter:
    return "counter";
  case MetricType::Gauge:
    return "gauge";
  default:
    return "unknown";
  }
}

std::unique_ptr<Metric> makeMetric(const MetricSpec &spec) {
  switch (parseMetricType(spec.type)) {
  case MetricType::Counter:
    return std::make_unique<CounterMetric>(spec);
  case MetricType::Gauge:
    return std::make_unique<GaugeMetric>(spec);
  default:
    return nullptr;
  }
}

}  // namespace simple_metricd
