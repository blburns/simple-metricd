/**
 * @file ingest.cpp
 */

#include "simple-metricd/http/ingest.hpp"

#include "simple-metricd/scrape/prometheus_text.hpp"
#include "simple-metricd/utils/logger.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace simple_metricd {

namespace {

std::string trim(std::string value) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

bool looksLikeSimpleLine(const std::string &line) {
  std::istringstream in(line);
  std::string name;
  std::string type;
  std::string value;
  if (!(in >> name >> type >> value)) {
    return false;
  }
  if (parseMetricType(type) == MetricType::Unknown) {
    return false;
  }
  // Remaining tokens optional; value must parse as double.
  try {
    (void)std::stod(value);
  } catch (...) {
    return false;
  }
  return true;
}

bool ingestSimpleLine(MetricRegistry &registry, const std::string &line, std::string &error) {
  std::istringstream in(line);
  MetricSpec spec;
  std::string value_token;
  if (!(in >> spec.name >> spec.type >> value_token)) {
    error = "expected name type value";
    return false;
  }
  try {
    spec.value = std::stod(value_token);
  } catch (...) {
    error = "invalid value";
    return false;
  }
  if (parseMetricType(spec.type) == MetricType::Unknown) {
    error = "unknown type: " + spec.type;
    return false;
  }
  if (!registry.upsert(spec)) {
    error = "failed to upsert " + spec.name;
    return false;
  }
  return true;
}

}  // namespace

IngestResult ingestBody(MetricRegistry &registry, const std::string &body) {
  IngestResult result;

  // Prefer Prometheus text when TYPE/HELP or braced labels are present.
  const bool prefer_prom =
      body.find("# TYPE ") != std::string::npos || body.find("# HELP ") != std::string::npos ||
      body.find('{') != std::string::npos;

  if (prefer_prom) {
    std::string error;
    const std::size_t merged = mergePrometheusText(registry, body, error);
    if (!error.empty()) {
      result.rejected = 1;
      result.last_error = error;
      Logger::instance().error("ingest error: " + error);
      return result;
    }
    result.accepted = merged;
    return result;
  }

  std::istringstream in(body);
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    line = trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::string error;
    if (looksLikeSimpleLine(line)) {
      if (ingestSimpleLine(registry, line, error)) {
        ++result.accepted;
      } else {
        ++result.rejected;
        result.last_error = error;
        Logger::instance().error("ingest error: " + error + " (" + line + ")");
      }
    } else {
      // Fall back to a single Prometheus sample line.
      std::vector<MetricSpec> specs;
      std::string parse_error;
      if (!parsePrometheusText(line + "\n", specs, parse_error) || specs.empty()) {
        ++result.rejected;
        result.last_error = parse_error.empty() ? "unrecognized ingest line" : parse_error;
        Logger::instance().error("ingest error: " + result.last_error + " (" + line + ")");
        continue;
      }
      if (registry.upsert(specs.front())) {
        ++result.accepted;
      } else {
        ++result.rejected;
        result.last_error = "failed to upsert " + specs.front().name;
        Logger::instance().error("ingest error: " + result.last_error);
      }
    }
  }
  return result;
}

}  // namespace simple_metricd
