/**
 * @file common.hpp
 */

#pragma once

#include <string>

namespace simple_metricd {
namespace cli {

struct ClientOptions {
  std::string config_path;
  std::string command{"list"};
  bool help{false};
  bool version{false};
  bool parse_error{false};
};

void printClientUsage();
void printVersion();
ClientOptions parseClientArgs(int argc, char *argv[]);

}  // namespace cli
}  // namespace simple_metricd
