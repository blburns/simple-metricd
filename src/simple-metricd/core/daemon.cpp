/**
 * @file daemon.cpp
 */

#include "simple-metricd/core/daemon.hpp"

#include "simple-metricd/utils/logger.hpp"
#include "simple-metricd/version.hpp"

namespace simple_metricd {

MetricDaemon::MetricDaemon(MetricConfig config) : config_(std::move(config)) {}

MetricDaemon::~MetricDaemon() { stop(); }

bool MetricDaemon::initialize() {
  if (initialized_.load()) {
    return true;
  }
  if (!config_.validate()) {
    Logger::instance().error("invalid configuration");
    return false;
  }
  LogLevel level = LogLevel::Info;
  if (parseLogLevel(config_.log_level, level)) {
    Logger::instance().setLevel(level);
  }
  if (!config_.log_file.empty()) {
    Logger::instance().setLogFile(config_.log_file);
  }
  metrics_.clear();
  for (const auto &spec : config_.metrics) {
    auto metric = makeMetric(spec);
    if (!metric) {
      Logger::instance().error("unsupported metric type: " + spec.type + " (" + spec.name + ")");
      return false;
    }
    metrics_.push_back(std::move(metric));
  }
  initialized_ = true;
  return true;
}

bool MetricDaemon::start() {
  if (!initialize()) {
    return false;
  }
  running_ = true;
  Logger::instance().info(std::string(kProjectName) + " " + kVersion + " started");
  Logger::instance().info("metrics configured: " + std::to_string(metrics_.size()));
  Logger::instance().info("HTTP /metrics exposition is not implemented in v0.1.0");
  return true;
}

void MetricDaemon::stop() {
  if (!running_.exchange(false)) {
    return;
  }
  Logger::instance().info("simple-metricd stopped");
}

bool MetricDaemon::running() const { return running_.load(); }

bool MetricDaemon::testConfig() const {
  std::vector<std::string> errors;
  if (!config_.validateDetailed(errors)) {
    for (const auto &error : errors) {
      Logger::instance().error(error);
    }
    return false;
  }
  Logger::instance().info("configuration is valid");
  return true;
}

}  // namespace simple_metricd
