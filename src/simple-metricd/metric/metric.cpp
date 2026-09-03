/**
 * @file metric.cpp
 */

#include "simple-metricd/metric/metric.hpp"

#include <algorithm>
#include <cctype>

namespace simple_metricd {

namespace {

std::string lowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

class StubMetric : public Metric {
public:
  explicit StubMetric(MetricSpec spec) : spec_(std::move(spec)) {}

  std::string name() const override { return spec_.name; }
  MetricType type() const override { return parseMetricType(spec_.type); }
  double value() const override { return spec_.value; }
  bool setValue(double) override { return false; }

private:
  MetricSpec spec_;
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
  if (parseMetricType(spec.type) == MetricType::Unknown) {
    return nullptr;
  }
  return std::make_unique<StubMetric>(spec);
}

}  // namespace simple_metricd
