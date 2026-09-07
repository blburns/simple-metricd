/**
 * @file metric.cpp
 */

#include "simple-metricd/metric/metric.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

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

std::vector<double> defaultHistogramBounds() {
  return {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0};
}

std::string joinBaseLabels(const std::string &labels, const std::string &extra) {
  if (labels.empty()) {
    return extra;
  }
  if (extra.empty()) {
    return labels;
  }
  return labels + "," + extra;
}

}  // namespace

HistogramMetric::HistogramMetric(MetricSpec spec, std::vector<double> upper_bounds)
    : name_(std::move(spec.name)), help_(std::move(spec.help)),
      labels_(std::move(spec.labels)), bounds_(std::move(upper_bounds)) {
  std::sort(bounds_.begin(), bounds_.end());
  bounds_.erase(std::unique(bounds_.begin(), bounds_.end()), bounds_.end());
  counts_.assign(bounds_.size(), 0);
}

std::string HistogramMetric::name() const { return name_; }
MetricType HistogramMetric::type() const { return MetricType::Histogram; }

double HistogramMetric::value() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<double>(count_);
}

bool HistogramMetric::setValue(double value) {
  if (!std::isfinite(value)) {
    return false;
  }
  observe(value);
  return true;
}

std::string HistogramMetric::help() const { return help_; }
std::string HistogramMetric::labels() const { return labels_; }

void HistogramMetric::observe(double sample) {
  if (!std::isfinite(sample)) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  for (std::size_t i = 0; i < bounds_.size(); ++i) {
    if (sample <= bounds_[i]) {
      ++counts_[i];
    }
  }
  sum_ += sample;
  ++count_;
}

double HistogramMetric::sum() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return sum_;
}

std::uint64_t HistogramMetric::count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return count_;
}

std::vector<std::uint64_t> HistogramMetric::bucketCounts() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return counts_;
}

void HistogramMetric::writeExposition(std::ostream &out, bool include_metadata) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (include_metadata) {
    if (!help_.empty()) {
      out << "# HELP " << name_ << " " << help_ << "\n";
    }
    out << "# TYPE " << name_ << " histogram\n";
  }
  // counts_[i] is cumulative: number of observations <= bounds_[i].
  for (std::size_t i = 0; i < bounds_.size(); ++i) {
    std::ostringstream le;
    le << "le=\"" << bounds_[i] << "\"";
    out << name_ << "_bucket{" << joinBaseLabels(labels_, le.str()) << "} " << counts_[i]
        << "\n";
  }
  out << name_ << "_bucket{" << joinBaseLabels(labels_, "le=\"+Inf\"") << "} " << count_
      << "\n";
  out << name_ << "_sum";
  if (!labels_.empty()) {
    out << "{" << labels_ << "}";
  }
  out << " " << sum_ << "\n";
  out << name_ << "_count";
  if (!labels_.empty()) {
    out << "{" << labels_ << "}";
  }
  out << " " << count_ << "\n";
}

std::optional<double> CounterRate::observe(double counter_value,
                                           std::chrono::steady_clock::time_point now) {
  if (!std::isfinite(counter_value)) {
    return std::nullopt;
  }
  if (!have_prev_) {
    prev_value_ = counter_value;
    prev_time_ = now;
    have_prev_ = true;
    last_rate_ = std::nullopt;
    return std::nullopt;
  }
  if (counter_value < prev_value_) {
    // Counter reset; re-baseline.
    prev_value_ = counter_value;
    prev_time_ = now;
    last_rate_ = std::nullopt;
    return std::nullopt;
  }
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::duration<double>>(now - prev_time_).count();
  if (elapsed <= 0.0) {
    return last_rate_;
  }
  last_rate_ = (counter_value - prev_value_) / elapsed;
  prev_value_ = counter_value;
  prev_time_ = now;
  return last_rate_;
}

void CounterRate::reset() {
  have_prev_ = false;
  prev_value_ = 0.0;
  last_rate_ = std::nullopt;
}

MetricType parseMetricType(const std::string &name) {
  const std::string lower = lowerCopy(name);
  if (lower == "counter") return MetricType::Counter;
  if (lower == "gauge") return MetricType::Gauge;
  if (lower == "histogram") return MetricType::Histogram;
  return MetricType::Unknown;
}

const char *toString(MetricType type) {
  switch (type) {
  case MetricType::Counter:
    return "counter";
  case MetricType::Gauge:
    return "gauge";
  case MetricType::Histogram:
    return "histogram";
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
  case MetricType::Histogram: {
    auto bounds = spec.buckets.empty() ? defaultHistogramBounds() : spec.buckets;
    return std::make_unique<HistogramMetric>(spec, std::move(bounds));
  }
  default:
    return nullptr;
  }
}

}  // namespace simple_metricd
