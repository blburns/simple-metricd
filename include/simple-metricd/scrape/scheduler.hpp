/**
 * @file scheduler.hpp
 * @brief Interval scrape scheduler for remote /metrics targets
 */

#pragma once

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/metric/registry.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace simple_metricd {

/**
 * Periodically HTTP GETs configured scrape targets and merges Prometheus text
 * into the local registry. Independent targets run on a worker pool so a slow
 * scrape does not block others (fair per-target scheduling).
 */
class ScrapeScheduler {
public:
  ScrapeScheduler(std::vector<ScrapeTarget> targets, MetricRegistry &registry,
                  std::size_t worker_count = 0);
  ~ScrapeScheduler();

  ScrapeScheduler(const ScrapeScheduler &) = delete;
  ScrapeScheduler &operator=(const ScrapeScheduler &) = delete;

  bool start();
  void stop();
  bool running() const { return running_.load(); }

  /** Number of successful scrape merges since start. */
  std::uint64_t successCount() const;
  /** Number of failed scrapes since start. */
  std::uint64_t failureCount() const;
  std::size_t workerCount() const { return workers_.size(); }

private:
  struct TargetState {
    ScrapeTarget target;
    std::chrono::steady_clock::time_point next_due{};
    bool in_flight{false};
  };

  void dispatcherLoop();
  void workerLoop();
  void scrapeOne(std::size_t index);

  std::vector<TargetState> targets_;
  MetricRegistry &registry_;
  std::size_t configured_workers_{0};
  std::atomic<bool> running_{false};
  std::thread dispatcher_;
  std::vector<std::thread> workers_;

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<std::size_t> queue_;

  mutable std::mutex stats_mutex_;
  std::uint64_t success_count_{0};
  std::uint64_t failure_count_{0};
};

}  // namespace simple_metricd
