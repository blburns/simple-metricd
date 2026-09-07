/**
 * @file daemon.cpp
 */

#include "simple-metricd/core/daemon.hpp"

#include "simple-metricd/security/acl.hpp"
#include "simple-metricd/utils/logger.hpp"
#include "simple-metricd/utils/net.hpp"
#include "simple-metricd/utils/privilege.hpp"
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
  if (config_.tls_enabled()) {
    if (!tls_.loadCertificate(config_.tls_cert_file, config_.tls_key_file)) {
      Logger::instance().error("failed to load TLS certificate/key");
      return false;
    }
    if (!config_.tls_ca_file.empty() && !tls_.loadCa(config_.tls_ca_file)) {
      Logger::instance().error("failed to load TLS CA file");
      return false;
    }
  }
  if (!config_.snapshot_file.empty()) {
    snapshot_ = std::make_unique<SnapshotStore>(config_.snapshot_file, registry_,
                                                config_.snapshot_interval_sec);
    if (!snapshot_->load()) {
      Logger::instance().error("failed to load snapshot: " + config_.snapshot_file);
      snapshot_.reset();
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
  if (tls_.enabled()) {
    server_->setTls(&tls_);
  }
  AclPolicy acl;
  acl.setAllow(config_.allow_ips);
  acl.setDeny(config_.deny_ips);
  acl.setCredentials(config_.auth_user, config_.auth_password);
  acl.setPublicHealthz(config_.public_healthz);
  server_->setAcl(acl);
  server_->setRateLimit(config_.rate_limit_per_minute);
  if (!server_->start()) {
    Logger::instance().error("failed to bind metrics HTTP listener on " + config_.listen_address +
                             ":" + std::to_string(config_.listen_port));
    server_.reset();
    return false;
  }
  if (!dropPrivileges(config_.run_as_user)) {
    Logger::instance().error("failed to drop privileges to user: " + config_.run_as_user);
    server_->stop();
    server_.reset();
    return false;
  }
  if (!config_.scrape_targets.empty()) {
    scraper_ = std::make_unique<ScrapeScheduler>(config_.scrape_targets, registry_);
    if (!scraper_->start()) {
      Logger::instance().error("failed to start scrape scheduler");
      server_->stop();
      server_.reset();
      scraper_.reset();
      return false;
    }
  }
  if (snapshot_ && !snapshot_->start()) {
    Logger::instance().error("failed to start snapshot store");
    if (scraper_) {
      scraper_->stop();
      scraper_.reset();
    }
    server_->stop();
    server_.reset();
    return false;
  }
  running_ = true;
  Logger::instance().info(std::string(kProjectName) + " " + kVersion + " started");
  Logger::instance().info("metrics registered: " + std::to_string(registry_.size()));
  Logger::instance().info(std::string("listening on ") + (tls_.enabled() ? "https://" : "http://") +
                          config_.listen_address + ":" + std::to_string(server_->boundPort()) +
                          " (/metrics /healthz /status)");
  if (!config_.scrape_targets.empty()) {
    Logger::instance().info("scrape targets: " + std::to_string(config_.scrape_targets.size()));
  }
  if (!config_.snapshot_file.empty()) {
    Logger::instance().info("snapshot file: " + config_.snapshot_file);
  }
  return true;
}

void MetricDaemon::stop() {
  if (!running_.exchange(false)) {
    if (snapshot_) {
      snapshot_->stop();
    }
    if (scraper_) {
      scraper_->stop();
      scraper_.reset();
    }
    if (server_) {
      server_->stop();
      server_.reset();
    }
    return;
  }
  if (snapshot_) {
    snapshot_->stop();
  }
  if (scraper_) {
    scraper_->stop();
    scraper_.reset();
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
