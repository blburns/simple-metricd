/**
 * @file test_ingest.cpp
 * @brief Push ingest endpoint tests
 */

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/core/daemon.hpp"
#include "simple-metricd/http/ingest.hpp"
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

void testIngestBodySimple() {
  MetricRegistry registry;
  const auto result = ingestBody(registry, "pushed_gauge gauge 3.5\nrequests_total counter 9\n");
  expect(result.accepted == 2 && result.rejected == 0, "simple ingest accepted");
  expect(registry.find("pushed_gauge") && registry.find("pushed_gauge")->value() == 3.5,
         "gauge value");
  expect(registry.find("requests_total") && registry.find("requests_total")->value() == 9.0,
         "counter value");
}

void testIngestBodyPrometheus() {
  MetricRegistry registry;
  const std::string body =
      "# TYPE remote_up gauge\n"
      "remote_up 1\n";
  const auto result = ingestBody(registry, body);
  expect(result.accepted == 1, "prom ingest accepted");
  expect(registry.find("remote_up") != nullptr, "remote_up present");
}

void testHttpIngestEndpoints() {
  MetricConfig config;
  config.listen_address = "127.0.0.1";
  config.listen_port = 0;
  config.metrics.push_back(MetricSpec{"simple_metricd_up", "gauge", "", 1.0, {}});

  MetricDaemon daemon(config);
  expect(daemon.start(), "daemon starts");
  if (!daemon.running()) {
    return;
  }
  const port_t port = daemon.boundPort();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto r1 = httpPost("127.0.0.1", port, "/ingest", "jobs_active gauge 2\n", 3000);
  expect(r1.status == 200, "/ingest HTTP 200");
  expect(r1.body.find("accepted=1") != std::string::npos, "/ingest accepted");
  expect(daemon.registry().find("jobs_active") &&
             daemon.registry().find("jobs_active")->value() == 2.0,
         "jobs_active upserted");

  const auto r2 =
      httpPost("127.0.0.1", port, "/api/v1/import", "batch_total counter 11\n", 3000);
  expect(r2.status == 200, "/api/v1/import HTTP 200");
  expect(daemon.registry().find("batch_total") != nullptr, "batch_total upserted");

  const auto bad = httpPost("127.0.0.1", port, "/ingest", "broken line only\n", 3000);
  expect(bad.status == 400, "bad ingest returns 400");

  daemon.stop();
}

}  // namespace

int main() {
  initializeSockets();
  testIngestBodySimple();
  testIngestBodyPrometheus();
  testHttpIngestEndpoints();
  if (g_failed != 0) {
    std::cout << g_failed << " ingest assertion(s) failed" << std::endl;
    return 1;
  }
  std::cout << "All ingest tests passed" << std::endl;
  return 0;
}
