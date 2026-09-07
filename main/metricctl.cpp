/**
 * @file metricctl.cpp
 */

#include "simple-metricd/cli/common.hpp"
#include "simple-metricd/config/config.hpp"
#include "simple-metricd/core/daemon.hpp"

#include <algorithm>
#include <iostream>

int main(int argc, char *argv[]) {
  const auto options = simple_metricd::cli::parseClientArgs(argc, argv);
  if (options.parse_error) {
    simple_metricd::cli::printClientUsage();
    return 1;
  }
  if (options.help) {
    simple_metricd::cli::printClientUsage();
    return 0;
  }
  if (options.version) {
    simple_metricd::cli::printVersion();
    return 0;
  }
  if (options.config_path.empty()) {
    std::cerr << options.command << " requires --config" << std::endl;
    return 1;
  }
  simple_metricd::MetricConfig config;
  if (!config.loadFromFile(options.config_path)) {
    std::cerr << "failed to load configuration" << std::endl;
    return 1;
  }
  if (options.command == "test-config") {
    simple_metricd::MetricDaemon daemon(config);
    return daemon.testConfig() ? 0 : 1;
  }
  if (options.command == "list") {
    simple_metricd::MetricDaemon daemon(config);
    if (!daemon.initialize()) {
      std::cerr << "failed to register metrics from configuration" << std::endl;
      return 1;
    }
    auto metrics = daemon.registry().list();
    std::sort(metrics.begin(), metrics.end(),
              [](const simple_metricd::Metric *a, const simple_metricd::Metric *b) {
                return a->name() < b->name();
              });
    for (const auto *metric : metrics) {
      std::cout << metric->name() << " " << simple_metricd::toString(metric->type()) << " "
                << metric->value() << std::endl;
    }
    return 0;
  }
  std::cerr << "unknown command: " << options.command << std::endl;
  return 2;
}
