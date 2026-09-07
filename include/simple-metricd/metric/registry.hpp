/**
 * @file registry.hpp
 * @brief In-memory counter/gauge metric registry
 */

#pragma once

#include "simple-metricd/metric/metric.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace simple_metricd {

class MetricRegistry {
public:
  /** Register a metric from a config spec. Fails if name empty, type unknown, or duplicate. */
  bool registerMetric(const MetricSpec &spec);

  /** Look up a registered metric by name (nullptr if missing). */
  Metric *find(const std::string &name);
  const Metric *find(const std::string &name) const;

  /** Set gauge freely; counters only accept non-decreasing values. */
  bool setValue(const std::string &name, double value);

  /** Increment a counter (delta must be >= 0). Gauges may use setValue. */
  bool increment(const std::string &name, double delta = 1.0);

  std::vector<Metric *> list();
  std::vector<const Metric *> list() const;

  std::size_t size() const;
  void clear();

private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::unique_ptr<Metric>> metrics_;
};

}  // namespace simple_metricd
