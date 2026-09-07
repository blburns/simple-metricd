/**
 * @file exposition.hpp
 * @brief Prometheus text exposition helpers
 */

#pragma once

#include "simple-metricd/metric/registry.hpp"
#include <string>

namespace simple_metricd {

/** Render Prometheus exposition format 0.0.4 text for the registry. */
std::string renderPrometheusText(const MetricRegistry &registry, bool include_metadata = true);

}  // namespace simple_metricd
