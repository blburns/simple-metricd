/**
 * @file rate_limiter.cpp
 */

#include "simple-metricd/security/rate_limiter.hpp"

namespace simple_metricd {

namespace {
constexpr auto kWindow = std::chrono::minutes(1);
}

RateLimiter::RateLimiter(std::uint32_t max_per_minute) : max_per_minute_(max_per_minute) {}

void RateLimiter::setMaxPerMinute(std::uint32_t max_per_minute) {
  std::lock_guard<std::mutex> lock(mutex_);
  max_per_minute_ = max_per_minute;
  buckets_.clear();
}

bool RateLimiter::allow(const std::string &client_id) const {
  if (max_per_minute_ == 0) {
    return true;
  }
  const auto now = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  auto &bucket = buckets_[client_id];
  if (bucket.window_start.time_since_epoch().count() == 0 ||
      now - bucket.window_start >= kWindow) {
    bucket.window_start = now;
    bucket.count = 0;
  }
  if (bucket.count >= max_per_minute_) {
    return false;
  }
  ++bucket.count;
  return true;
}

}  // namespace simple_metricd
