/**
 * @file test_acl.cpp
 * @brief Unit tests for metrics ACL policy
 */

#include "simple-metricd/security/acl.hpp"

#include <iostream>

using namespace simple_metricd;

namespace {

int g_failed = 0;

void expect(bool cond, const char *msg) {
  if (!cond) {
    std::cout << "FAIL " << msg << std::endl;
    ++g_failed;
  } else {
    std::cout << "PASS " << msg << std::endl;
  }
}

}  // namespace

int main() {
  AclPolicy acl;
  acl.setAllow({"127.0.0.1"});
  acl.setDeny({"10.0.0.*"});
  expect(acl.allow("127.0.0.1:4000", "/metrics", ""), "allow loopback");
  expect(!acl.allow("10.0.0.5:1", "/metrics", ""), "deny 10.0.0.*");
  expect(!acl.allow("192.168.1.1:1", "/metrics", ""), "reject non-allowlisted");
  expect(!acl.allow("10.0.0.5:1", "/healthz", ""), "deny still blocks healthz");

  AclPolicy auth;
  auth.setCredentials("ops", "secret");
  auth.setPublicHealthz(true);
  expect(auth.allow("1.2.3.4:1", "/healthz", ""), "healthz open without auth");
  expect(!auth.allow("1.2.3.4:1", "/metrics", ""), "metrics requires auth");
  expect(auth.allow("1.2.3.4:1", "/metrics", "Basic ops:secret"), "metrics with basic auth");

  if (g_failed != 0) {
    std::cout << g_failed << " assertion(s) failed" << std::endl;
    return 1;
  }
  std::cout << "ACL tests passed" << std::endl;
  return 0;
}
