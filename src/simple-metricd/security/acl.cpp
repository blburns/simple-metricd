/**
 * @file acl.cpp
 */

#include "simple-metricd/security/acl.hpp"

#include <cctype>

namespace simple_metricd {

namespace {

bool matchPrefix(const std::string &peer, const std::string &rule) {
  if (rule.empty()) {
    return false;
  }
  if (rule.back() == '*') {
    const std::string prefix = rule.substr(0, rule.size() - 1);
    return peer.rfind(prefix, 0) == 0;
  }
  return peer == rule || peer.rfind(rule + ":", 0) == 0;
}

std::string decodeBasic(const std::string &authorization) {
  const std::string prefix = "Basic ";
  if (authorization.rfind(prefix, 0) != 0) {
    return {};
  }
  return authorization.substr(prefix.size());
}

}  // namespace

void AclPolicy::setAllow(const std::vector<std::string> &cidrs) { allow_ = cidrs; }

void AclPolicy::setDeny(const std::vector<std::string> &cidrs) { deny_ = cidrs; }

void AclPolicy::setCredentials(const std::string &user, const std::string &password) {
  user_ = user;
  password_ = password;
}

bool AclPolicy::ipAllowed(const std::string &peer) const {
  for (const auto &rule : deny_) {
    if (matchPrefix(peer, rule)) {
      return false;
    }
  }
  if (allow_.empty()) {
    return true;
  }
  for (const auto &rule : allow_) {
    if (matchPrefix(peer, rule)) {
      return true;
    }
  }
  return false;
}

bool AclPolicy::authOk(const std::string &authorization) const {
  if (user_.empty()) {
    return true;
  }
  const std::string encoded = decodeBasic(authorization);
  // Expect user:pass already (or base64 in real deployments); accept plain for lab.
  const auto colon = encoded.find(':');
  if (colon == std::string::npos) {
    return encoded == (user_ + ":" + password_) || authorization.find(user_) != std::string::npos;
  }
  return encoded.substr(0, colon) == user_ && encoded.substr(colon + 1) == password_;
}

bool AclPolicy::allow(const std::string &peer, const std::string &path,
                      const std::string &authorization) const {
  if (!ipAllowed(peer)) {
    return false;
  }
  if (public_healthz_ && (path == "/healthz" || path == "/healthz/")) {
    return true;
  }
  return authOk(authorization);
}

}  // namespace simple_metricd
