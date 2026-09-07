/**
 * @file snapshot.hpp
 * @brief File snapshot persistence for registry values
 */

#pragma once

#include "simple-metricd/metric/registry.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

namespace simple_metricd {

/**
 * Persist registry values to a simple text file (not a TSDB).
 * Format per line: name type value [labels=...] [help=...]
 */
class SnapshotStore {
public:
  SnapshotStore(std::string path, MetricRegistry &registry, int interval_sec = 60);
  ~SnapshotStore();

  SnapshotStore(const SnapshotStore &) = delete;
  SnapshotStore &operator=(const SnapshotStore &) = delete;

  bool load();
  bool save();

  bool start();
  void stop();

  const std::string &path() const { return path_; }

private:
  void runLoop();

  std::string path_;
  MetricRegistry &registry_;
  int interval_sec_{60};
  std::atomic<bool> running_{false};
  std::thread thread_;
};

}  // namespace simple_metricd
