/**
 * @file metric.hpp
 * @brief Pluggable metric interface
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <vector>
#include <chrono>

namespace simple_metricd {

enum class MetricType { Counter, Gauge, Histogram, Unknown };

struct MetricSpec {
  std::string name;
  std::string type;
  std::string help;
  double value{0.0};
  std::string labels;  // Prometheus body without braces
  /** Upper bounds for histogram buckets (exclusive of +Inf). */
  std::vector<double> buckets;

  MetricSpec() = default;
  MetricSpec(std::string name_in, std::string type_in, std::string help_in = {},
             double value_in = 0.0, std::string labels_in = {},
             std::vector<double> buckets_in = {})
      : name(std::move(name_in)), type(std::move(type_in)), help(std::move(help_in)),
        value(value_in), labels(std::move(labels_in)), buckets(std::move(buckets_in)) {}
};

MetricType parseMetricType(const std::string &name);
const char *toString(MetricType type);

class Metric {
public:
  virtual ~Metric() = default;
  virtual std::string name() const = 0;
  virtual MetricType type() const = 0;
  virtual double value() const = 0;
  virtual bool setValue(double value) = 0;
  virtual std::string help() const { return {}; }
  /** Prometheus label set body, e.g. `job="api",instance="a"` (no braces). */
  virtual std::string labels() const { return {}; }
};

/**
 * Cumulative histogram with fixed upper bounds (+Inf implied).
 * Exposition emits _bucket/_sum/_count in Prometheus text format.
 */
class HistogramMetric : public Metric {
public:
  HistogramMetric(MetricSpec spec, std::vector<double> upper_bounds);

  std::string name() const override;
  MetricType type() const override;
  /** Returns the observation count. */
  double value() const override;
  bool setValue(double value) override;
  std::string help() const override;
  std::string labels() const override;

  void observe(double sample);
  double sum() const;
  std::uint64_t count() const;
  const std::vector<double> &bounds() const { return bounds_; }
  std::vector<std::uint64_t> bucketCounts() const;

  void writeExposition(std::ostream &out, bool include_metadata) const;

private:
  std::string name_;
  std::string help_;
  std::string labels_;
  std::vector<double> bounds_;
  mutable std::mutex mutex_;
  std::vector<std::uint64_t> counts_;
  double sum_{0.0};
  std::uint64_t count_{0};
};

/**
 * Simple counter rate helper: units/sec between successive observations.
 * Resets baseline when the counter decreases (restart).
 */
class CounterRate {
public:
  std::optional<double> observe(
      double counter_value,
      std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
  std::optional<double> rate() const { return last_rate_; }
  void reset();

private:
  bool have_prev_{false};
  double prev_value_{0.0};
  std::chrono::steady_clock::time_point prev_time_{};
  std::optional<double> last_rate_;
};

std::unique_ptr<Metric> makeMetric(const MetricSpec &spec);

}  // namespace simple_metricd
