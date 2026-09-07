/**
 * @file tls.cpp
 */

#include "simple-metricd/security/tls.hpp"

namespace simple_metricd {

bool TlsContext::load(const std::string &, const std::string &, const std::string &) {
  enabled_ = false;
  return false;
}

}  // namespace simple_metricd
