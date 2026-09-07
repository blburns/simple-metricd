/**
 * @file privilege.cpp
 */

#include "simple-metricd/utils/privilege.hpp"

#ifndef SIMPLE_METRICD_WINDOWS
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#endif

namespace simple_metricd {

bool dropPrivileges(const std::string &user) {
  if (user.empty()) {
    return true;
  }
#ifndef SIMPLE_METRICD_WINDOWS
  struct passwd *pw = getpwnam(user.c_str());
  if (!pw) {
    return false;
  }
  if (setgid(pw->pw_gid) != 0) {
    return false;
  }
  if (setuid(pw->pw_uid) != 0) {
    return false;
  }
  return true;
#else
  (void)user;
  return true;
#endif
}

}  // namespace simple_metricd
