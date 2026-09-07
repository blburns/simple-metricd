/**
 * @file scheduler.cpp
 */

#include "simple-metricd/scrape/scheduler.hpp"

#include "simple-metricd/scrape/prometheus_text.hpp"
#include "simple-metricd/utils/logger.hpp"
#include "simple-metricd/utils/net.hpp"

#include <algorithm>

namespace simple_metricd {

ScrapeScheduler::ScrapeScheduler(std::vector<ScrapeTarget> targets, MetricRegistry &registry)
    : targets_(std::move(targets)), registry_(registry) {}

ScrapeScheduler::~ScrapeScheduler() { stop(); }

bool ScrapeScheduler::start() {
  if (targets_.empty()) {
    return true;
  }
  if (running_.exchange(true)) {
    return true;
  }
  next_due_.assign(targets_.size(), std::chrono::steady_clock::now());
  thread_ = std::thread([this]() { runLoop(); });
  return true;
}

void ScrapeScheduler::stop() {
  if (!running_.exchange(false)) {
    if (thread_.joinable()) {
      thread_.join();
    }
    return;
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

std::uint64_t ScrapeScheduler::successCount() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  return success_count_;
}

std::uint64_t ScrapeScheduler::failureCount() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  return failure_count_;
}

void ScrapeScheduler::scrapeOne(const ScrapeTarget &target) {
  const int timeout_ms = std::max(1, target.timeout_sec) * 1000;
  const auto response = httpGet(target.host, target.port, target.path, timeout_ms);
  if (response.status != 200) {
    std::string detail = response.error.empty()
                             ? ("HTTP " + std::to_string(response.status))
                             : response.error;
    Logger::instance().warning("scrape failed " + target.host + ":" +
                               std::to_string(target.port) + target.path + ": " + detail);
    std::lock_guard<std::mutex> lock(stats_mutex_);
    ++failure_count_;
    return;
  }
  std::string error;
  const std::size_t merged = mergePrometheusText(registry_, response.body, error);
  if (!error.empty()) {
    Logger::instance().warning("scrape merge error for " + target.host + ":" +
                               std::to_string(target.port) + target.path + ": " + error);
    std::lock_guard<std::mutex> lock(stats_mutex_);
    ++failure_count_;
    return;
  }
  Logger::instance().debug("scraped " + target.host + ":" + std::to_string(target.port) +
                           target.path + " merged=" + std::to_string(merged));
  std::lock_guard<std::mutex> lock(stats_mutex_);
  ++success_count_;
}

void ScrapeScheduler::runLoop() {
  using clock = std::chrono::steady_clock;
  while (running_) {
    const auto now = clock::now();
    bool scraped_any = false;
    for (std::size_t i = 0; i < targets_.size() && running_; ++i) {
      if (now < next_due_[i]) {
        continue;
      }
      scrapeOne(targets_[i]);
      next_due_[i] = clock::now() + std::chrono::seconds(std::max(1, targets_[i].interval_sec));
      scraped_any = true;
    }
    if (!running_) {
      break;
    }
    if (!scraped_any) {
      auto soonest = next_due_.empty() ? now + std::chrono::seconds(1) : next_due_[0];
      for (const auto &due : next_due_) {
        soonest = std::min(soonest, due);
      }
      auto sleep_for = soonest - clock::now();
      if (sleep_for > std::chrono::milliseconds(200)) {
        sleep_for = std::chrono::milliseconds(200);
      }
      if (sleep_for > std::chrono::milliseconds(0)) {
        std::this_thread::sleep_for(sleep_for);
      }
    }
  }
}

}  // namespace simple_metricd
