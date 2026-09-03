/**
 * @file config.hpp
 */

#pragma once

#include "simple-metricd/metric/metric.hpp"
#include "simple-metricd/utils/platform.hpp"
#include <string>
#include <vector>

namespace simple_metricd {

class MetricConfig {
public:
  MetricConfig();

  bool loadFromFile(const std::string &path);
  bool validate() const;
  bool validateDetailed(std::vector<std::string> &errors) const;

  std::string listen_address{"0.0.0.0"};
  port_t listen_port{kMetricsDefaultPort};
  std::string log_file;
  std::string log_level{"info"};
  bool foreground{true};
  std::vector<MetricSpec> metrics;
  std::vector<std::string> metric_errors;
};

}  // namespace simple_metricd
