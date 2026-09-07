/**
 * @file test_metric_registry.cpp
 * @brief Unit tests for the in-memory metric registry
 */

#include "simple-metricd/metric/registry.hpp"

#include <chrono>
#include <cmath>
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

void testRegisterAndList() {
  MetricRegistry registry;
  MetricSpec up{"simple_metricd_up", "gauge", "daemon up", 1.0, {}};
  MetricSpec req{"requests_total", "counter", "requests", 0.0, {}};
  expect(registry.registerMetric(up), "register gauge");
  expect(registry.registerMetric(req), "register counter");
  expect(!registry.registerMetric(up), "reject duplicate name");
  expect(registry.size() == 2, "size is 2");
  expect(registry.list().size() == 2, "list size is 2");
}

void testCounterSemantics() {
  MetricRegistry registry;
  MetricSpec req{"requests_total", "counter", "", 5.0, {}};
  expect(registry.registerMetric(req), "register counter at 5");
  expect(registry.find("requests_total")->value() == 5.0, "initial counter value");
  expect(registry.increment("requests_total", 2.0), "increment by 2");
  expect(registry.find("requests_total")->value() == 7.0, "counter is 7");
  expect(!registry.increment("requests_total", -1.0), "reject negative increment");
  expect(!registry.setValue("requests_total", 3.0), "reject counter decrease");
  expect(registry.setValue("requests_total", 10.0), "allow counter increase via set");
  expect(registry.find("requests_total")->value() == 10.0, "counter is 10");
}

void testGaugeSemantics() {
  MetricRegistry registry;
  MetricSpec temp{"temperature_celsius", "gauge", "", 20.0, {}};
  expect(registry.registerMetric(temp), "register gauge");
  expect(registry.setValue("temperature_celsius", 18.5), "set gauge down");
  expect(std::abs(registry.find("temperature_celsius")->value() - 18.5) < 1e-9, "gauge is 18.5");
  expect(registry.setValue("temperature_celsius", 25.0), "set gauge up");
  expect(!registry.increment("temperature_celsius", 1.0), "increment rejects gauges");
}

void testUnknownType() {
  MetricRegistry registry;
  MetricSpec bad{"weird", "flux", "", 0.0, {}};
  expect(!registry.registerMetric(bad), "reject unknown type");
  expect(registry.size() == 0, "empty after reject");
}

void testHistogramAndRate() {
  MetricRegistry registry;
  MetricSpec hist{"latency_seconds", "histogram", "request latency", 0.0, {}, {0.1, 0.5, 1.0}};
  expect(registry.registerMetric(hist), "register histogram");
  auto *metric = dynamic_cast<HistogramMetric *>(registry.find("latency_seconds"));
  expect(metric != nullptr, "histogram type");
  metric->observe(0.05);
  metric->observe(0.4);
  metric->observe(2.0);
  expect(metric->count() == 3, "histogram count");
  expect(std::abs(metric->sum() - 2.45) < 1e-9, "histogram sum");
  const auto buckets = metric->bucketCounts();
  expect(buckets.size() == 3 && buckets[0] == 1 && buckets[1] == 2 && buckets[2] == 2,
         "cumulative buckets");

  CounterRate rate;
  const auto t0 = std::chrono::steady_clock::now();
  expect(!rate.observe(10.0, t0).has_value(), "rate needs two samples");
  const auto r = rate.observe(20.0, t0 + std::chrono::seconds(2));
  expect(r.has_value() && std::abs(*r - 5.0) < 1e-9, "rate is 5/sec");
}

}  // namespace

int main() {
  testRegisterAndList();
  testCounterSemantics();
  testGaugeSemantics();
  testUnknownType();
  testHistogramAndRate();
  if (g_failed != 0) {
    std::cout << "Registry tests failed: " << g_failed << std::endl;
    return 1;
  }
  std::cout << "Registry tests: all passed" << std::endl;
  return 0;
}
