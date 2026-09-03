/**
 * @file common.cpp
 */

#include "simple-metricd/cli/common.hpp"
#include "simple-metricd/utils/platform.hpp"
#include "simple-metricd/version.hpp"

#include <iostream>

namespace simple_metricd {
namespace cli {

void printClientUsage() {
  std::cout << "Usage: metricctl [OPTIONS] COMMAND\n\n"
            << "Options:\n"
            << "  --help, -h           Show this help message\n"
            << "  --version, -v        Show version information\n"
            << "  --config, -c FILE    Use specified configuration file\n"
            << "  --test-config        Validate configuration and exit\n\n"
            << "Commands:\n"
            << "  list                 List configured metrics from a config file\n";
}

void printVersion() {
  std::cout << "metricctl " << kVersion << std::endl;
  std::cout << kDescription << std::endl;
  std::cout << "Platform: " << platformName() << std::endl;
}

ClientOptions parseClientArgs(int argc, char *argv[]) {
  ClientOptions options;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      options.help = true;
    } else if (arg == "--version" || arg == "-v") {
      options.version = true;
    } else if (arg == "--test-config") {
      options.command = "test-config";
    } else if (arg == "--config" || arg == "-c") {
      if (i + 1 >= argc) {
        options.parse_error = true;
      } else {
        options.config_path = argv[++i];
      }
    } else if (!arg.empty() && arg[0] != '-') {
      options.command = arg;
    } else {
      options.parse_error = true;
    }
  }
  return options;
}

}  // namespace cli
}  // namespace simple_metricd
