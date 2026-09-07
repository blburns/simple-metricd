/**
 * @file metrics_server.hpp
 * @brief HTTP metrics/health endpoints
 */

#pragma once

#include "simple-metricd/metric/registry.hpp"
#include "simple-metricd/security/acl.hpp"
#include "simple-metricd/security/rate_limiter.hpp"
#include "simple-metricd/security/tls.hpp"
#include "simple-metricd/utils/net.hpp"
#include "simple-metricd/utils/platform.hpp"
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace simple_metricd {

class MetricsServer {
public:
  MetricsServer(std::string listen_address, port_t listen_port, MetricRegistry &registry);
  ~MetricsServer();

  MetricsServer(const MetricsServer &) = delete;
  MetricsServer &operator=(const MetricsServer &) = delete;

  void setTls(TlsContext *tls) { tls_ = tls; }
  void setAcl(const AclPolicy &acl) { acl_ = acl; }
  void setRateLimit(std::uint32_t max_per_minute) { rate_limiter_.setMaxPerMinute(max_per_minute); }

  bool start();
  void stop();
  port_t boundPort() const { return bound_port_; }
  bool running() const { return running_.load(); }

private:
  void acceptLoop();
  void handleClient(TcpConnection connection);

  std::string listen_address_;
  port_t listen_port_{0};
  MetricRegistry &registry_;
  TlsContext *tls_{nullptr};
  AclPolicy acl_;
  RateLimiter rate_limiter_;
  TcpListener listener_;
  std::atomic<bool> running_{false};
  std::thread thread_;
  port_t bound_port_{0};
};

}  // namespace simple_metricd
