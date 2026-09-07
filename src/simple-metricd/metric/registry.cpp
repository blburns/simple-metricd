/**
 * @file registry.cpp
 * @brief In-memory counter/gauge metric registry
 */

#include "simple-metricd/metric/registry.hpp"

#include <utility>

namespace simple_metricd {

bool MetricRegistry::registerMetric(const MetricSpec &spec) {
  if (spec.name.empty()) {
    return false;
  }
  auto metric = makeMetric(spec);
  if (!metric) {
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (metrics_.find(spec.name) != metrics_.end()) {
    return false;
  }
  metrics_.emplace(spec.name, std::move(metric));
  return true;
}

bool MetricRegistry::upsert(const MetricSpec &spec) {
  if (spec.name.empty()) {
    return false;
  }
  auto metric = makeMetric(spec);
  if (!metric) {
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  metrics_[spec.name] = std::move(metric);
  return true;
}

Metric *MetricRegistry::find(const std::string &name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = metrics_.find(name);
  return it == metrics_.end() ? nullptr : it->second.get();
}

const Metric *MetricRegistry::find(const std::string &name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = metrics_.find(name);
  return it == metrics_.end() ? nullptr : it->second.get();
}

bool MetricRegistry::setValue(const std::string &name, double value) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = metrics_.find(name);
  if (it == metrics_.end()) {
    return false;
  }
  return it->second->setValue(value);
}

bool MetricRegistry::increment(const std::string &name, double delta) {
  if (delta < 0.0) {
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = metrics_.find(name);
  if (it == metrics_.end()) {
    return false;
  }
  Metric *metric = it->second.get();
  if (metric->type() != MetricType::Counter) {
    return false;
  }
  return metric->setValue(metric->value() + delta);
}

std::vector<Metric *> MetricRegistry::list() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<Metric *> out;
  out.reserve(metrics_.size());
  for (auto &entry : metrics_) {
    out.push_back(entry.second.get());
  }
  return out;
}

std::vector<const Metric *> MetricRegistry::list() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<const Metric *> out;
  out.reserve(metrics_.size());
  for (const auto &entry : metrics_) {
    out.push_back(entry.second.get());
  }
  return out;
}

std::size_t MetricRegistry::size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return metrics_.size();
}

void MetricRegistry::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  metrics_.clear();
}

}  // namespace simple_metricd
