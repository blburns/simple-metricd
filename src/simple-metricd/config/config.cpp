/**
 * @file config.cpp
 */

#include "simple-metricd/config/config.hpp"
#include "simple-metricd/utils/logger.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace simple_metricd {

namespace {

std::string trim(std::string value) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

bool parseBool(const std::string &value) {
  std::string lower = value;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

std::vector<std::string> splitList(const std::string &value) {
  std::vector<std::string> out;
  std::string token;
  std::istringstream in(value);
  while (std::getline(in, token, ',')) {
    token = trim(token);
    if (!token.empty()) {
      out.push_back(token);
    }
  }
  return out;
}

bool parseMetricLine(const std::string &value, MetricSpec &spec, std::string &error) {
  std::istringstream in(value);
  if (!(in >> spec.name >> spec.type)) {
    error = "metric needs name and type";
    return false;
  }

  std::string rest;
  std::getline(in, rest);
  rest = trim(rest);

  auto isKeyAt = [&](std::size_t pos, const char *key) {
    const std::size_t n = std::char_traits<char>::length(key);
    return rest.compare(pos, n, key) == 0;
  };

  std::size_t pos = 0;
  while (pos < rest.size()) {
    while (pos < rest.size() && std::isspace(static_cast<unsigned char>(rest[pos]))) {
      ++pos;
    }
    if (pos >= rest.size()) {
      break;
    }
    if (isKeyAt(pos, "help=")) {
      pos += 5;
      std::size_t end = pos;
      while (end < rest.size()) {
        if (end > pos && std::isspace(static_cast<unsigned char>(rest[end - 1])) &&
            (isKeyAt(end, "labels=") || isKeyAt(end, "value="))) {
          break;
        }
        ++end;
      }
      spec.help = trim(rest.substr(pos, end - pos));
      pos = end;
      continue;
    }
    if (isKeyAt(pos, "labels=")) {
      pos += 7;
      std::size_t end = pos;
      while (end < rest.size() && !std::isspace(static_cast<unsigned char>(rest[end]))) {
        ++end;
      }
      spec.labels = rest.substr(pos, end - pos);
      pos = end;
      continue;
    }
    if (isKeyAt(pos, "value=")) {
      pos += 6;
      std::size_t end = pos;
      while (end < rest.size() && !std::isspace(static_cast<unsigned char>(rest[end]))) {
        ++end;
      }
      try {
        spec.value = std::stod(rest.substr(pos, end - pos));
      } catch (...) {
        error = "invalid value=";
        return false;
      }
      pos = end;
      continue;
    }
    std::size_t end = pos;
    while (end < rest.size() && !std::isspace(static_cast<unsigned char>(rest[end]))) {
      ++end;
    }
    try {
      spec.value = std::stod(rest.substr(pos, end - pos));
    } catch (...) {
      error = "metric trailing field must be a number or help=/labels=/value=";
      return false;
    }
    pos = end;
  }

  if (spec.name.empty() || parseMetricType(spec.type) == MetricType::Unknown) {
    error = "unknown metric type: " + spec.type;
    return false;
  }
  return true;
}

}  // namespace

MetricConfig::MetricConfig() = default;

bool MetricConfig::loadFromFile(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  std::string line;
  while (std::getline(in, line)) {
    auto comment = line.find('#');
    if (comment != std::string::npos) {
      line = line.substr(0, comment);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = trim(line.substr(0, eq));
    const std::string value = trim(line.substr(eq + 1));
    if (key == "listen_address") {
      listen_address = value;
    } else if (key == "listen_port") {
      listen_port = static_cast<port_t>(std::stoi(value));
    } else if (key == "log_file") {
      log_file = value;
    } else if (key == "log_level") {
      log_level = value;
    } else if (key == "foreground") {
      foreground = parseBool(value);
    } else if (key == "tls_cert_file" || key == "tls_cert") {
      tls_cert_file = value;
    } else if (key == "tls_key_file" || key == "tls_key") {
      tls_key_file = value;
    } else if (key == "tls_ca_file" || key == "tls_ca") {
      tls_ca_file = value;
    } else if (key == "allow_ip" || key == "allow_ips") {
      allow_ips = splitList(value);
    } else if (key == "deny_ip" || key == "deny_ips") {
      deny_ips = splitList(value);
    } else if (key == "auth_user") {
      auth_user = value;
    } else if (key == "auth_password") {
      auth_password = value;
    } else if (key == "public_healthz") {
      public_healthz = parseBool(value);
    } else if (key == "metric") {
      MetricSpec spec;
      std::string error;
      if (!parseMetricLine(value, spec, error)) {
        metric_errors.push_back(error.empty() ? "invalid metric" : error);
      } else {
        metrics.push_back(std::move(spec));
      }
    }
  }
  return true;
}

bool MetricConfig::validate() const {
  std::vector<std::string> errors;
  return validateDetailed(errors);
}

bool MetricConfig::validateDetailed(std::vector<std::string> &errors) const {
  errors.clear();
  if (listen_address.empty()) {
    errors.emplace_back("listen_address is required");
  }
  LogLevel parsed_level = LogLevel::Info;
  if (!parseLogLevel(log_level, parsed_level)) {
    errors.emplace_back("log_level must be debug, info, warning, error, or fatal");
  }
  for (const auto &error : metric_errors) {
    errors.push_back(error);
  }
  if (tls_cert_file.empty() != tls_key_file.empty()) {
    errors.emplace_back("tls_cert_file and tls_key_file must both be set");
  }
  if (!auth_user.empty() && auth_password.empty()) {
    errors.emplace_back("auth_password is required when auth_user is set");
  }
  return errors.empty();
}

}  // namespace simple_metricd
