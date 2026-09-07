/**
 * @file scheduler.hpp
 * @brief Interval scrape scheduler for remote /metrics targets
 */

#pragma once

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/metric/registry.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace simple_metricd {

/**
 * Periodically HTTP GETs configured scrape targets and merges Prometheus text
 * into the local registry. Milestone 6 runs targets sequentially on one thread.
 */
class ScrapeScheduler {
public:
  ScrapeScheduler(std::vector<ScrapeTarget> targets, MetricRegistry &registry);
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

private:
  void runLoop();
  void scrapeOne(const ScrapeTarget &target);

  std::vector<ScrapeTarget> targets_;
  MetricRegistry &registry_;
  std::atomic<bool> running_{false};
  std::thread thread_;
  mutable std::mutex stats_mutex_;
  std::uint64_t success_count_{0};
  std::uint64_t failure_count_{0};
  std::vector<std::chrono::steady_clock::time_point> next_due_;
};

}  // namespace simple_metricd
