/**
 * @file config.hpp
 */

#pragma once

#include "simple-metricd/metric/metric.hpp"
#include "simple-metricd/utils/platform.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace simple_metricd {

/** Remote Prometheus text scrape target. */
struct ScrapeTarget {
  std::string host;
  port_t port{80};
  std::string path{"/metrics"};
  int interval_sec{15};
  int timeout_sec{5};
};

class MetricConfig {
public:
  MetricConfig();

  bool loadFromFile(const std::string &path);
  bool validate() const;
  bool validateDetailed(std::vector<std::string> &errors) const;

  std::string listen_address{"0.0.0.0"};
  port_t listen_port{kMetricsDefaultPort};
  std::string log_file;
  std::string log_level{"info"};
  bool foreground{true};
  std::vector<MetricSpec> metrics;
  std::vector<std::string> metric_errors;
  std::vector<ScrapeTarget> scrape_targets;
  std::vector<std::string> scrape_errors;
  /** Optional registry snapshot path (load on start, save periodically/on stop). */
  std::string snapshot_file;
  int snapshot_interval_sec{60};

  // TLS (Milestone 5)
  std::string tls_cert_file;
  std::string tls_key_file;
  std::string tls_ca_file;
  bool tls_enabled() const { return !tls_cert_file.empty() && !tls_key_file.empty(); }

  // Early ACL / optional auth (pulled forward from Milestone 11)
  std::vector<std::string> allow_ips;
  std::vector<std::string> deny_ips;
  std::string auth_user;
  std::string auth_password;
  bool public_healthz{true};

  // Hardening (Milestone 12)
  std::uint32_t rate_limit_per_minute{0};  // 0 = disabled
  std::string run_as_user;
};

}  // namespace simple_metricd
