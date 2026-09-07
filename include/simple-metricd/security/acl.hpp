/**
 * @file acl.hpp
 * @brief Metrics endpoint IP ACL and optional basic auth
 */

#pragma once

#include <string>
#include <vector>

namespace simple_metricd {

class AclPolicy {
public:
  void setAllow(const std::vector<std::string> &cidrs);
  void setDeny(const std::vector<std::string> &cidrs);
  void setCredentials(const std::string &user, const std::string &password);
  bool publicHealthz() const { return public_healthz_; }
  void setPublicHealthz(bool value) { public_healthz_ = value; }

  bool allow(const std::string &peer, const std::string &path,
             const std::string &authorization) const;

private:
  bool ipAllowed(const std::string &peer) const;
  bool authOk(const std::string &authorization) const;

  std::vector<std::string> allow_;
  std::vector<std::string> deny_;
  std::string user_;
  std::string password_;
  bool public_healthz_{true};
};

}  // namespace simple_metricd
