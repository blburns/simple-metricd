/**
 * @file platform.cpp
 * @brief Platform name helper
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-metricd/utils/platform.hpp"

namespace simple_metricd {

std::string platformName() {
#ifdef SIMPLE_METRICD_WINDOWS
  return "Windows";
#elif defined(SIMPLE_METRICD_MACOS)
  return "macOS";
#elif defined(SIMPLE_METRICD_FREEBSD)
  return "FreeBSD";
#elif defined(SIMPLE_METRICD_LINUX)
  return "Linux";
#else
  return "Unknown";
#endif
}

}  // namespace simple_metricd
