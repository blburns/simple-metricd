/**
 * @file test_metrics_http.cpp
 * @brief Smoke tests for /metrics and /healthz
 */

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/core/daemon.hpp"
#include "simple-metricd/http/exposition.hpp"
#include "simple-metricd/metric/registry.hpp"
#include "simple-metricd/utils/net.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace simple_metricd;

namespace {

int g_failed = 0;

void expect(bool cond, const char *msg) {
  if (!cond) {
    std::cout << "FAIL " << msg << std::endl;
    ++g_failed;
  } else {
    std::cout << "PASS " << msg << std::endl;
  }
}

void testExpositionText() {
  MetricRegistry registry;
  MetricSpec up{"simple_metricd_up", "gauge", "daemon up", 1.0, {}};
  MetricSpec req{"requests_total", "counter", "requests", 3.0, "job=\"lab\""};
  expect(registry.registerMetric(up), "register gauge for exposition");
  expect(registry.registerMetric(req), "register counter for exposition");
  const std::string text = renderPrometheusText(registry, true);
  expect(text.find("# HELP simple_metricd_up daemon up") != std::string::npos, "HELP line");
  expect(text.find("# TYPE simple_metricd_up gauge") != std::string::npos, "TYPE line");
  expect(text.find("simple_metricd_up 1") != std::string::npos, "gauge sample");
  expect(text.find("requests_total{job=\"lab\"} 3") != std::string::npos, "labeled sample");
}

void testHttpSmoke() {
  MetricConfig config;
  config.listen_address = "127.0.0.1";
  config.listen_port = 19201;
  config.metrics.push_back(MetricSpec{"simple_metricd_up", "gauge", "up", 1.0, {}});
  config.metrics.push_back(MetricSpec{"requests_total", "counter", "requests", 2.0, {}});

  MetricDaemon daemon(config);
  expect(daemon.start(), "daemon starts HTTP listener");
  if (!daemon.running()) {
    return;
  }
  const port_t port = daemon.boundPort();
  expect(port == 19201, "bound listen port");

  // Allow accept thread to start.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto metrics = httpGet("127.0.0.1", port, "/metrics", 3000);
  expect(metrics.status == 200, "/metrics HTTP 200");
  expect(metrics.body.find("simple_metricd_up") != std::string::npos, "/metrics body has gauge");
  expect(metrics.body.find("# TYPE") != std::string::npos, "/metrics has TYPE");

  const auto health = httpGet("127.0.0.1", port, "/healthz", 3000);
  expect(health.status == 200, "/healthz HTTP 200");
  expect(health.body.find("ok") != std::string::npos, "/healthz body");

  const auto status = httpGet("127.0.0.1", port, "/status", 3000);
  expect(status.status == 200, "/status HTTP 200");
  expect(status.body.find("\"metrics\":2") != std::string::npos, "/status metric count");

  const auto missing = httpGet("127.0.0.1", port, "/nope", 3000);
  expect(missing.status == 404, "unknown path 404");

  daemon.stop();
}

}  // namespace

int main() {
  initializeSockets();
  testExpositionText();
  testHttpSmoke();
  if (g_failed != 0) {
    std::cout << g_failed << " assertion(s) failed" << std::endl;
    return 1;
  }
  std::cout << "All metrics HTTP tests passed" << std::endl;
  return 0;
}
