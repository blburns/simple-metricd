/**
 * @file daemon.hpp
 */

#pragma once

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/http/metrics_server.hpp"
#include "simple-metricd/metric/metric.hpp"
#include "simple-metricd/metric/registry.hpp"
#include "simple-metricd/scrape/scheduler.hpp"
#include "simple-metricd/security/tls.hpp"
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
  ScrapeScheduler *scraper() { return scraper_.get(); }
  port_t boundPort() const;

private:
  MetricConfig config_;
  MetricRegistry registry_;
  TlsContext tls_;
  std::unique_ptr<MetricsServer> server_;
  std::unique_ptr<ScrapeScheduler> scraper_;
  std::atomic<bool> running_{false};
  std::atomic<bool> initialized_{false};
};

}  // namespace simple_metricd
