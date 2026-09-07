/**
 * @file prometheus_text.hpp
 * @brief Parse Prometheus text and merge samples into a registry
 */

#pragma once

#include "simple-metricd/metric/metric.hpp"
#include "simple-metricd/metric/registry.hpp"
#include <string>
#include <vector>

namespace simple_metricd {

/**
 * Parse Prometheus exposition text (HELP/TYPE/sample lines) into MetricSpec entries.
 * Unknown TYPE values are stored as gauge so scrape merge can still accept them.
 */
bool parsePrometheusText(const std::string &text, std::vector<MetricSpec> &out,
                         std::string &error);

/** Upsert every parsed sample into the registry. Returns number merged. */
std::size_t mergePrometheusText(MetricRegistry &registry, const std::string &text,
                                std::string &error);

}  // namespace simple_metricd
