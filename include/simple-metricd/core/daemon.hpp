/**
 * @file daemon.hpp
 */

#pragma once

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/metric/metric.hpp"
#include <atomic>
#include <memory>
#include <vector>

namespace simple_metricd {

class MetricDaemon {
public:
  explicit MetricDaemon(MetricConfig config);
  ~MetricDaemon();

  MetricDaemon(const MetricDaemon &) = delete;
  MetricDaemon &operator=(const MetricDaemon &) = delete;

  bool initialize();
  bool start();
  void stop();
  bool running() const;
  bool testConfig() const;

  const MetricConfig &config() const { return config_; }
  const std::vector<std::unique_ptr<Metric>> &metrics() const { return metrics_; }

private:
  MetricConfig config_;
  std::vector<std::unique_ptr<Metric>> metrics_;
  std::atomic<bool> running_{false};
  std::atomic<bool> initialized_{false};
};

}  // namespace simple_metricd
