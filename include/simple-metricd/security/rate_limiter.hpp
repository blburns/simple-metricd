/**
 * @file rate_limiter.hpp
 * @brief Per-client metrics API rate limiter
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace simple_metricd {

class RateLimiter {
public:
  explicit RateLimiter(std::uint32_t max_per_minute = 0);

  void setMaxPerMinute(std::uint32_t max_per_minute);
  bool allow(const std::string &client_id) const;

private:
  struct Bucket {
    std::chrono::steady_clock::time_point window_start{};
    std::uint32_t count{0};
  };

  std::uint32_t max_per_minute_{0};
  mutable std::mutex mutex_;
  mutable std::map<std::string, Bucket> buckets_;
};

}  // namespace simple_metricd
