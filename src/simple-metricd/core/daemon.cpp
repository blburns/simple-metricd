/**
 * @file daemon.cpp
 */

#include "simple-metricd/core/daemon.hpp"

#include "simple-metricd/utils/logger.hpp"
#include "simple-metricd/utils/net.hpp"
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
  registry_.clear();
  for (const auto &spec : config_.metrics) {
    if (!registry_.registerMetric(spec)) {
      Logger::instance().error("failed to register metric: " + spec.name + " (" + spec.type + ")");
      return false;
    }
  }
  initialized_ = true;
  return true;
}

bool MetricDaemon::start() {
  if (!initialize()) {
    return false;
  }
  if (!initializeSockets()) {
    Logger::instance().error("failed to initialize sockets");
    return false;
  }
  server_ = std::make_unique<MetricsServer>(config_.listen_address, config_.listen_port, registry_);
  if (!server_->start()) {
    Logger::instance().error("failed to bind metrics HTTP listener on " + config_.listen_address +
                             ":" + std::to_string(config_.listen_port));
    server_.reset();
    return false;
  }
  running_ = true;
  Logger::instance().info(std::string(kProjectName) + " " + kVersion + " started");
  Logger::instance().info("metrics registered: " + std::to_string(registry_.size()));
  Logger::instance().info("listening on " + config_.listen_address + ":" +
                          std::to_string(server_->boundPort()) + " (/metrics /healthz /status)");
  return true;
}

void MetricDaemon::stop() {
  if (!running_.exchange(false)) {
    if (server_) {
      server_->stop();
      server_.reset();
    }
    return;
  }
  if (server_) {
    server_->stop();
    server_.reset();
  }
  Logger::instance().info("simple-metricd stopped");
}

bool MetricDaemon::running() const { return running_.load(); }

port_t MetricDaemon::boundPort() const {
  return server_ ? server_->boundPort() : config_.listen_port;
}

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
