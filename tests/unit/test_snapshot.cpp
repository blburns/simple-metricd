/**
 * @file test_snapshot.cpp
 * @brief File snapshot persistence tests
 */

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/core/daemon.hpp"
#include "simple-metricd/metric/registry.hpp"
#include "simple-metricd/metric/snapshot.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>

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

void testSaveAndLoad() {
  const std::string path = "test-metricd-snapshot.txt";
  std::remove(path.c_str());

  MetricRegistry registry;
  expect(registry.registerMetric(MetricSpec{"up", "gauge", "daemon up", 1.0, "job=\"lab\""}),
         "register gauge");
  expect(registry.registerMetric(MetricSpec{"requests_total", "counter", "", 42.0, {}}),
         "register counter");

  SnapshotStore store(path, registry, 60);
  expect(store.save(), "save snapshot");

  MetricRegistry restored;
  SnapshotStore loader(path, restored, 60);
  expect(loader.load(), "load snapshot");
  expect(restored.size() == 2, "restored size");
  expect(restored.find("up") && restored.find("up")->value() == 1.0, "up restored");
  expect(restored.find("up")->labels() == "job=\"lab\"", "labels restored");
  expect(restored.find("requests_total") &&
             restored.find("requests_total")->value() == 42.0,
         "counter restored");

  std::remove(path.c_str());
}

void testDaemonLoadOnStart() {
  const std::string path = "test-metricd-daemon-snapshot.txt";
  {
    std::ofstream out(path);
    out << "persisted_gauge gauge 8.25 labels=env=\"test\" help=from snapshot\n";
  }

  MetricConfig config;
  config.listen_address = "127.0.0.1";
  config.listen_port = 0;
  config.snapshot_file = path;
  config.snapshot_interval_sec = 3600;
  config.metrics.push_back(MetricSpec{"simple_metricd_up", "gauge", "", 1.0, {}});

  MetricDaemon daemon(config);
  expect(daemon.initialize(), "daemon initialize loads snapshot");
  expect(daemon.registry().find("persisted_gauge") != nullptr, "persisted metric present");
  expect(daemon.registry().find("persisted_gauge")->value() == 8.25, "persisted value");
  expect(daemon.registry().find("simple_metricd_up") != nullptr, "config metric retained");

  // Overwrite config metric value from a later snapshot upsert of same name.
  expect(daemon.start(), "daemon start");
  daemon.registry().upsert(MetricSpec{"runtime_metric", "counter", "", 3.0, {}});
  expect(daemon.snapshot() && daemon.snapshot()->save(), "manual save");
  daemon.stop();

  MetricConfig reload = config;
  MetricDaemon daemon2(reload);
  expect(daemon2.initialize(), "reload initialize");
  expect(daemon2.registry().find("runtime_metric") &&
             daemon2.registry().find("runtime_metric")->value() == 3.0,
         "runtime metric survived stop/save");

  std::remove(path.c_str());
}

void testConfigSnapshotKey() {
  const std::string path = "test-snapshot-config.conf";
  std::ofstream out(path);
  out << "snapshot_file = /tmp/simple-metricd.snap\n";
  out << "snapshot_interval = 30\n";
  out.close();
  MetricConfig config;
  expect(config.loadFromFile(path), "load snapshot config");
  expect(config.snapshot_file == "/tmp/simple-metricd.snap", "snapshot_file parsed");
  expect(config.snapshot_interval_sec == 30, "snapshot_interval parsed");
  expect(config.validate(), "config valid");
}

}  // namespace

int main() {
  testSaveAndLoad();
  testDaemonLoadOnStart();
  testConfigSnapshotKey();
  if (g_failed != 0) {
    std::cout << g_failed << " snapshot assertion(s) failed" << std::endl;
    return 1;
  }
  std::cout << "All snapshot tests passed" << std::endl;
  return 0;
}
