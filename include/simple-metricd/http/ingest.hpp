/**
 * @file ingest.hpp
 * @brief Push ingest of simple and Prometheus sample lines
 */

#pragma once

#include "simple-metricd/metric/registry.hpp"
#include <cstddef>
#include <string>

namespace simple_metricd {

struct IngestResult {
  std::size_t accepted{0};
  std::size_t rejected{0};
  std::string last_error;
};

/**
 * Accept body lines of form `name type value` or Prometheus sample lines
 * (optionally preceded by # TYPE / # HELP). Updates the registry via upsert.
 */
IngestResult ingestBody(MetricRegistry &registry, const std::string &body);

}  // namespace simple_metricd
