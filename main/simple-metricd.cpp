/**
 * @file simple-metricd.cpp
 * @brief Metrics daemon entry point
 */

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/core/daemon.hpp"
#include "simple-metricd/utils/logger.hpp"
#include "simple-metricd/utils/platform.hpp"
#include "simple-metricd/version.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

namespace {

std::atomic<bool> g_shutdown{false};

void handleSignal(int) { g_shutdown = true; }

void printUsage() {
  std::cout << "Usage: simple-metricd [OPTIONS] [COMMAND]\n\n"
            << "Options:\n"
            << "  --help, -h           Show this help message\n"
            << "  --version, -v        Show version information\n"
            << "  --config, -c FILE    Use specified configuration file\n"
            << "  --foreground, -f     Run in the foreground\n"
            << "  --daemon, -d         Run as a daemon (not yet implemented)\n"
            << "  --test-config        Validate configuration and exit\n\n"
            << "Commands:\n"
            << "  start                Start the metrics daemon\n"
            << "  stop                 Stop the metrics daemon (not implemented)\n"
            << "  reload               Reload configuration (not implemented)\n";
}

void printVersion() {
  std::cout << simple_metricd::kProjectName << " " << simple_metricd::kVersion << std::endl;
  std::cout << simple_metricd::kDescription << std::endl;
  std::cout << "Platform: " << simple_metricd::platformName() << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
  simple_metricd::MetricConfig config;
  std::string command = "start";
  bool test_config = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printUsage();
      return 0;
    }
    if (arg == "--version" || arg == "-v") {
      printVersion();
      return 0;
    }
    if (arg == "--config" || arg == "-c") {
      if (i + 1 >= argc || !config.loadFromFile(argv[++i])) {
        std::cerr << "failed to load configuration" << std::endl;
        return 1;
      }
    } else if (arg == "--foreground" || arg == "-f") {
      config.foreground = true;
    } else if (arg == "--daemon" || arg == "-d") {
      config.foreground = false;
    } else if (arg == "--test-config") {
      test_config = true;
    } else if (!arg.empty() && arg[0] != '-') {
      command = arg;
    }
  }

  simple_metricd::MetricDaemon daemon(config);
  if (test_config || command == "test") {
    return daemon.testConfig() ? 0 : 1;
  }
  if (command == "stop" || command == "reload") {
    std::cerr << command << " is not implemented in this skeleton" << std::endl;
    return 2;
  }

  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
  if (!daemon.start()) {
    return 1;
  }
  if (!config.foreground) {
    simple_metricd::Logger::instance().warning(
        "daemonize is not implemented; staying in the foreground");
  }
  while (!g_shutdown && daemon.running()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  daemon.stop();
  return 0;
}
