/**
 * @file test_metric_config.cpp
 */

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/utils/logger.hpp"
#include "simple-metricd/version.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

using namespace simple_metricd;

namespace {

std::string writeTempConfig() {
  const std::string path = "test-simple-metricd.conf";
  std::ofstream out(path);
  out << "listen_address = 127.0.0.1\n";
  out << "listen_port = 19100\n";
  out << "metric = simple_metricd_up gauge help=daemon process up labels=job=\"lab\"\n";
  out << "metric = requests_total counter 0\n";
  return path;
}

bool testDefaultConfig() {
  MetricConfig config;
  assert(config.listen_port == kMetricsDefaultPort);
  assert(config.validate());
  return true;
}

bool testFileLoad() {
  const auto path = writeTempConfig();
  MetricConfig config;
  if (!config.loadFromFile(path)) {
    return false;
  }
  return config.listen_port == 19100 && config.listen_address == "127.0.0.1" &&
         config.metrics.size() == 2 && config.metrics[0].name == "simple_metricd_up" &&
         config.metrics[0].help == "daemon process up" &&
         config.metrics[0].labels == "job=\"lab\"" &&
         config.metrics[1].type == "counter" && config.validate();
}

bool testInvalidLogLevel() {
  MetricConfig config;
  config.log_level = "verbose";
  return !config.validate();
}

bool testUnknownMetricType() {
  const std::string path = "test-simple-metricd-bad-metric.conf";
  std::ofstream out(path);
  out << "metric = weird flux capacitor\n";
  out.close();
  MetricConfig config;
  return config.loadFromFile(path) && !config.validate();
}

bool testLogLevelParseAndApply() {
  const std::string path = "test-simple-metricd-log.conf";
  std::ofstream out(path);
  out << "log_level = warning\n";
  out.close();
  MetricConfig config;
  LogLevel level = LogLevel::Info;
  if (!config.loadFromFile(path) || config.log_level != "warning" ||
      !parseLogLevel(config.log_level, level) || level != LogLevel::Warning ||
      !config.validate()) {
    return false;
  }
  Logger::instance().setLevel(level);
  const bool applied = Logger::instance().level() == LogLevel::Warning;
  Logger::instance().setLevel(LogLevel::Info);
  return applied && kVersion[0] != '\0';
}

}  // namespace

int main() {
  int passed = 0;
  int total = 0;
  auto run = [&](const char *name, bool (*fn)()) {
    ++total;
    if (fn()) {
      ++passed;
      std::cout << "PASS " << name << std::endl;
    } else {
      std::cout << "FAIL " << name << std::endl;
    }
  };
  run("testDefaultConfig", testDefaultConfig);
  run("testFileLoad", testFileLoad);
  run("testInvalidLogLevel", testInvalidLogLevel);
  run("testUnknownMetricType", testUnknownMetricType);
  run("testLogLevelParseAndApply", testLogLevelParseAndApply);
  std::cout << "Config tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
