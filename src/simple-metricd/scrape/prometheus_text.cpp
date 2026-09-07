/**
 * @file prometheus_text.cpp
 */

#include "simple-metricd/scrape/prometheus_text.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace simple_metricd {

namespace {

std::string trim(std::string value) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

bool parseSampleLine(const std::string &line, MetricSpec &spec, std::string &error) {
  spec = MetricSpec{};
  std::size_t pos = 0;
  while (pos < line.size() &&
         (std::isalnum(static_cast<unsigned char>(line[pos])) || line[pos] == '_' ||
          line[pos] == ':')) {
    ++pos;
  }
  if (pos == 0) {
    error = "missing metric name";
    return false;
  }
  spec.name = line.substr(0, pos);

  if (pos < line.size() && line[pos] == '{') {
    const auto end = line.find('}', pos);
    if (end == std::string::npos) {
      error = "unclosed label set";
      return false;
    }
    spec.labels = line.substr(pos + 1, end - pos - 1);
    pos = end + 1;
  }

  while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
    ++pos;
  }
  if (pos >= line.size()) {
    error = "missing sample value";
    return false;
  }

  std::size_t value_end = pos;
  while (value_end < line.size() && !std::isspace(static_cast<unsigned char>(line[value_end]))) {
    ++value_end;
  }
  try {
    spec.value = std::stod(line.substr(pos, value_end - pos));
  } catch (...) {
    error = "invalid sample value";
    return false;
  }
  return true;
}

}  // namespace

bool parsePrometheusText(const std::string &text, std::vector<MetricSpec> &out,
                         std::string &error) {
  out.clear();
  error.clear();
  std::unordered_map<std::string, std::string> types;
  std::unordered_map<std::string, std::string> helps;

  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (line.rfind("# HELP ", 0) == 0) {
      const std::string rest = trim(line.substr(7));
      const auto sp = rest.find(' ');
      if (sp == std::string::npos) {
        continue;
      }
      helps[rest.substr(0, sp)] = trim(rest.substr(sp + 1));
      continue;
    }
    if (line.rfind("# TYPE ", 0) == 0) {
      const std::string rest = trim(line.substr(7));
      std::istringstream ts(rest);
      std::string name;
      std::string type;
      if (ts >> name >> type) {
        types[name] = type;
      }
      continue;
    }
    if (line[0] == '#') {
      continue;
    }

    MetricSpec spec;
    std::string sample_error;
    if (!parseSampleLine(line, spec, sample_error)) {
      error = sample_error;
      return false;
    }
    auto type_it = types.find(spec.name);
    if (type_it != types.end()) {
      spec.type = type_it->second;
    } else {
      spec.type = "gauge";
    }
    // Until full remote histogram reassembly, map unsupported scraped types to gauge.
    if (parseMetricType(spec.type) == MetricType::Unknown) {
      spec.type = "gauge";
    }
    auto help_it = helps.find(spec.name);
    if (help_it != helps.end()) {
      spec.help = help_it->second;
    }
    out.push_back(std::move(spec));
  }
  return true;
}

std::size_t mergePrometheusText(MetricRegistry &registry, const std::string &text,
                                std::string &error) {
  std::vector<MetricSpec> specs;
  if (!parsePrometheusText(text, specs, error)) {
    return 0;
  }
  std::size_t merged = 0;
  for (const auto &spec : specs) {
    if (registry.upsert(spec)) {
      ++merged;
    }
  }
  return merged;
}

}  // namespace simple_metricd
