/**
 * @file daemon.hpp
 */

#pragma once

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/metric/metric.hpp"
#include "simple-metricd/metric/registry.hpp"
#include <atomic>
#include <memory>

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
  MetricRegistry &registry() { return registry_; }
  const MetricRegistry &registry() const { return registry_; }

private:
  MetricConfig config_;
  MetricRegistry registry_;
  std::atomic<bool> running_{false};
  std::atomic<bool> initialized_{false};
};

}  // namespace simple_metricd
