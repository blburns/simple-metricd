/**
 * @file scheduler.cpp
 */

#include "simple-metricd/scrape/scheduler.hpp"

#include "simple-metricd/scrape/prometheus_text.hpp"
#include "simple-metricd/utils/logger.hpp"
#include "simple-metricd/utils/net.hpp"

#include <algorithm>

namespace simple_metricd {

ScrapeScheduler::ScrapeScheduler(std::vector<ScrapeTarget> targets, MetricRegistry &registry,
                                 std::size_t worker_count)
    : registry_(registry), configured_workers_(worker_count) {
  targets_.reserve(targets.size());
  for (auto &target : targets) {
    TargetState state;
    state.target = std::move(target);
    state.next_due = std::chrono::steady_clock::now();
    targets_.push_back(std::move(state));
  }
}

ScrapeScheduler::~ScrapeScheduler() { stop(); }

bool ScrapeScheduler::start() {
  if (targets_.empty()) {
    return true;
  }
  if (running_.exchange(true)) {
    return true;
  }

  std::size_t workers = configured_workers_;
  if (workers == 0) {
    const std::size_t hw = std::max<std::size_t>(2, std::thread::hardware_concurrency());
    workers = std::min(targets_.size(), std::min<std::size_t>(hw, 8));
  }
  workers = std::max<std::size_t>(1, workers);

  workers_.reserve(workers);
  for (std::size_t i = 0; i < workers; ++i) {
    workers_.emplace_back([this]() { workerLoop(); });
  }
  dispatcher_ = std::thread([this]() { dispatcherLoop(); });
  return true;
}

void ScrapeScheduler::stop() {
  if (!running_.exchange(false)) {
    if (dispatcher_.joinable()) {
      dispatcher_.join();
    }
    for (auto &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
    workers_.clear();
    return;
  }
  cv_.notify_all();
  if (dispatcher_.joinable()) {
    dispatcher_.join();
  }
  for (auto &worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  workers_.clear();
  std::lock_guard<std::mutex> lock(mutex_);
  queue_.clear();
}

std::uint64_t ScrapeScheduler::successCount() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  return success_count_;
}

std::uint64_t ScrapeScheduler::failureCount() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  return failure_count_;
}

void ScrapeScheduler::scrapeOne(std::size_t index) {
  const ScrapeTarget target = [&]() {
    std::lock_guard<std::mutex> lock(mutex_);
    return targets_[index].target;
  }();

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
  } else {
    std::string error;
    const std::size_t merged = mergePrometheusText(registry_, response.body, error);
    if (!error.empty()) {
      Logger::instance().warning("scrape merge error for " + target.host + ":" +
                                 std::to_string(target.port) + target.path + ": " + error);
      std::lock_guard<std::mutex> lock(stats_mutex_);
      ++failure_count_;
    } else {
      Logger::instance().debug("scraped " + target.host + ":" + std::to_string(target.port) +
                               target.path + " merged=" + std::to_string(merged));
      std::lock_guard<std::mutex> lock(stats_mutex_);
      ++success_count_;
    }
  }

  std::lock_guard<std::mutex> lock(mutex_);
  targets_[index].in_flight = false;
  targets_[index].next_due =
      std::chrono::steady_clock::now() +
      std::chrono::seconds(std::max(1, targets_[index].target.interval_sec));
}

void ScrapeScheduler::dispatcherLoop() {
  using clock = std::chrono::steady_clock;
  while (running_) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      const auto now = clock::now();
      for (std::size_t i = 0; i < targets_.size(); ++i) {
        auto &state = targets_[i];
        if (state.in_flight || now < state.next_due) {
          continue;
        }
        state.in_flight = true;
        queue_.push_back(i);
      }
    }
    cv_.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  cv_.notify_all();
}

void ScrapeScheduler::workerLoop() {
  while (true) {
    std::size_t index = 0;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [&]() { return !running_ || !queue_.empty(); });
      if (!running_ && queue_.empty()) {
        return;
      }
      if (queue_.empty()) {
        continue;
      }
      index = queue_.front();
      queue_.pop_front();
    }
    scrapeOne(index);
  }
}

}  // namespace simple_metricd
