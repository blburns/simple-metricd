/**
 * @file privilege.hpp
 * @brief Drop privileges to a service user
 */

#pragma once

#include <string>

namespace simple_metricd {

/** Drop to the named user when non-empty. Returns true on success or when unused. */
bool dropPrivileges(const std::string &user);

}  // namespace simple_metricd
