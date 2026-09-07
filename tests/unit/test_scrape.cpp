/**
 * @file test_scrape.cpp
 * @brief Unit/smoke tests for scrape config, Prometheus merge, and scheduler
 */

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/core/daemon.hpp"
#include "simple-metricd/scrape/prometheus_text.hpp"
#include "simple-metricd/scrape/scheduler.hpp"
#include "simple-metricd/utils/net.hpp"

#include <chrono>
#include <fstream>
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

void testParseScrapeTarget() {
  const std::string path = "test-scrape-target.conf";
  std::ofstream out(path);
  out << "listen_address = 127.0.0.1\n";
  out << "scrape_target = 127.0.0.1:9100/metrics interval=15 timeout=5\n";
  out << "scrape_target = localhost:19100 interval=30 timeout=2\n";
  out.close();

  MetricConfig config;
  expect(config.loadFromFile(path), "load scrape config");
  expect(config.scrape_targets.size() == 2, "two scrape targets");
  expect(config.scrape_targets[0].host == "127.0.0.1", "host 0");
  expect(config.scrape_targets[0].port == 9100, "port 0");
  expect(config.scrape_targets[0].path == "/metrics", "path 0");
  expect(config.scrape_targets[0].interval_sec == 15, "interval 0");
  expect(config.scrape_targets[0].timeout_sec == 5, "timeout 0");
  expect(config.scrape_targets[1].path == "/metrics", "default path");
  expect(config.scrape_targets[1].interval_sec == 30, "interval 1");
  expect(config.validate(), "scrape config valid");
}

void testParseAndMergePrometheus() {
  const std::string text =
      "# HELP up daemon up\n"
      "# TYPE up gauge\n"
      "up 1\n"
      "# TYPE requests_total counter\n"
      "requests_total{job=\"lab\"} 7\n";

  std::vector<MetricSpec> specs;
  std::string error;
  expect(parsePrometheusText(text, specs, error), "parse prometheus text");
  expect(error.empty(), "no parse error");
  expect(specs.size() == 2, "two samples");
  expect(specs[0].name == "up" && specs[0].type == "gauge", "up gauge");
  expect(specs[1].labels == "job=\"lab\"", "labels preserved");

  MetricRegistry registry;
  const std::size_t merged = mergePrometheusText(registry, text, error);
  expect(merged == 2, "merged two");
  expect(registry.find("up") && registry.find("up")->value() == 1.0, "up value");
  expect(registry.find("requests_total") &&
             registry.find("requests_total")->value() == 7.0,
         "counter value");
  expect(registry.find("requests_total")->labels() == "job=\"lab\"", "merged labels");
}

void testLiveScrapeMerge() {
  MetricConfig source_cfg;
  source_cfg.listen_address = "127.0.0.1";
  source_cfg.listen_port = 0;
  source_cfg.metrics.push_back(MetricSpec{"remote_up", "gauge", "remote", 1.0, {}});
  source_cfg.metrics.push_back(MetricSpec{"remote_requests", "counter", "", 4.0, {}});

  MetricDaemon source(source_cfg);
  expect(source.start(), "source daemon starts");
  if (!source.running()) {
    return;
  }
  const port_t source_port = source.boundPort();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  MetricConfig sink_cfg;
  sink_cfg.listen_address = "127.0.0.1";
  sink_cfg.listen_port = 0;
  sink_cfg.metrics.push_back(MetricSpec{"local_up", "gauge", "", 1.0, {}});
  ScrapeTarget target;
  target.host = "127.0.0.1";
  target.port = source_port;
  target.path = "/metrics";
  target.interval_sec = 1;
  target.timeout_sec = 2;
  sink_cfg.scrape_targets.push_back(target);

  MetricDaemon sink(sink_cfg);
  expect(sink.start(), "sink daemon starts");
  if (!sink.running()) {
    source.stop();
    return;
  }

  bool merged = false;
  for (int i = 0; i < 40; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (sink.registry().find("remote_up") &&
        sink.registry().find("remote_requests")) {
      merged = true;
      break;
    }
  }
  expect(merged, "scrape merged remote metrics");
  if (merged) {
    expect(sink.registry().find("remote_up")->value() == 1.0, "remote_up value");
    expect(sink.registry().find("remote_requests")->value() == 4.0, "remote_requests value");
    expect(sink.registry().find("local_up") != nullptr, "local metric retained");
  }
  if (sink.scraper()) {
    expect(sink.scraper()->successCount() >= 1, "scrape success counted");
  }

  sink.stop();
  source.stop();
}

void testParallelFairScheduling() {
  MetricConfig fast_cfg;
  fast_cfg.listen_address = "127.0.0.1";
  fast_cfg.listen_port = 0;
  fast_cfg.metrics.push_back(MetricSpec{"fast_up", "gauge", "", 1.0, {}});
  MetricDaemon fast(fast_cfg);
  expect(fast.start(), "fast source starts");
  if (!fast.running()) {
    return;
  }
  const port_t fast_port = fast.boundPort();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  MetricRegistry registry;
  std::vector<ScrapeTarget> targets;

  ScrapeTarget good;
  good.host = "127.0.0.1";
  good.port = fast_port;
  good.path = "/metrics";
  good.interval_sec = 1;
  good.timeout_sec = 1;
  targets.push_back(good);

  // Blackhole port: concurrent timeout must not starve the good target.
  ScrapeTarget slow;
  slow.host = "127.0.0.1";
  slow.port = 1;
  slow.path = "/metrics";
  slow.interval_sec = 1;
  slow.timeout_sec = 2;
  targets.push_back(slow);

  ScrapeScheduler scheduler(targets, registry, /*worker_count=*/2);
  expect(scheduler.start(), "parallel scheduler starts");
  expect(scheduler.workerCount() == 2, "two workers");

  bool got_fast = false;
  std::uint64_t successes = 0;
  for (int i = 0; i < 60; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (registry.find("fast_up")) {
      got_fast = true;
    }
    successes = scheduler.successCount();
    if (got_fast && successes >= 2) {
      break;
    }
  }
  expect(got_fast, "fast target scraped despite slow peer");
  expect(successes >= 2, "fast target scraped more than once while slow times out");
  expect(scheduler.failureCount() >= 1, "slow target recorded failures");

  scheduler.stop();
  fast.stop();
}

}  // namespace

int main() {
  initializeSockets();
  testParseScrapeTarget();
  testParseAndMergePrometheus();
  testLiveScrapeMerge();
  testParallelFairScheduling();
  if (g_failed != 0) {
    std::cout << g_failed << " scrape assertion(s) failed" << std::endl;
    return 1;
  }
  std::cout << "All scrape tests passed" << std::endl;
  return 0;
}
