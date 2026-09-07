/**
 * @file metric.hpp
 * @brief Pluggable metric interface
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace simple_metricd {

enum class MetricType { Counter, Gauge, Unknown };

struct MetricSpec {
  std::string name;
  std::string type;
  std::string help;
  double value{0.0};
  std::string labels;  // Prometheus body without braces
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

std::unique_ptr<Metric> makeMetric(const MetricSpec &spec);

}  // namespace simple_metricd
