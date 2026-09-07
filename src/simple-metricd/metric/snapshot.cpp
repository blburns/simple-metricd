/**
 * @file snapshot.cpp
 */

#include "simple-metricd/metric/snapshot.hpp"

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

bool parseSnapshotLine(const std::string &line, MetricSpec &spec, std::string &error) {
  std::istringstream in(line);
  if (!(in >> spec.name >> spec.type)) {
    error = "snapshot line needs name and type";
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
  bool have_value = false;
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
        have_value = true;
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
      have_value = true;
    } catch (...) {
      error = "snapshot trailing field must be a number or help=/labels=/value=";
      return false;
    }
    pos = end;
  }

  if (!have_value) {
    error = "snapshot line missing value";
    return false;
  }
  if (spec.name.empty() || parseMetricType(spec.type) == MetricType::Unknown) {
    error = "unknown metric type: " + spec.type;
    return false;
  }
  return true;
}

}  // namespace

SnapshotStore::SnapshotStore(std::string path, MetricRegistry &registry, int interval_sec)
    : path_(std::move(path)), registry_(registry),
      interval_sec_(interval_sec > 0 ? interval_sec : 60) {}

SnapshotStore::~SnapshotStore() { stop(); }

bool SnapshotStore::load() {
  if (path_.empty()) {
    return true;
  }
  std::ifstream in(path_);
  if (!in) {
    Logger::instance().info("snapshot file not found (starting empty): " + path_);
    return true;
  }
  std::string line;
  std::size_t loaded = 0;
  while (std::getline(in, line)) {
    auto comment = line.find('#');
    if (comment != std::string::npos) {
      line = line.substr(0, comment);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    MetricSpec spec;
    std::string error;
    if (!parseSnapshotLine(line, spec, error)) {
      Logger::instance().warning("snapshot skip: " + error);
      continue;
    }
    if (registry_.upsert(spec)) {
      ++loaded;
    }
  }
  Logger::instance().info("loaded " + std::to_string(loaded) + " metrics from snapshot " + path_);
  return true;
}

bool SnapshotStore::save() {
  if (path_.empty()) {
    return true;
  }
  const std::string tmp = path_ + ".tmp";
  std::ofstream out(tmp, std::ios::trunc);
  if (!out) {
    Logger::instance().error("failed to write snapshot temp file: " + tmp);
    return false;
  }
  out << "# simple-metricd registry snapshot\n";
  auto metrics = registry_.list();
  std::sort(metrics.begin(), metrics.end(),
            [](const Metric *a, const Metric *b) { return a->name() < b->name(); });
  for (const Metric *metric : metrics) {
    // Histograms persist as their primary value; full bucket restore is best-effort.
    out << metric->name() << " " << toString(metric->type()) << " " << metric->value();
    const std::string labels = metric->labels();
    if (!labels.empty()) {
      out << " labels=" << labels;
    }
    const std::string help = metric->help();
    if (!help.empty()) {
      out << " help=" << help;
    }
    out << "\n";
  }
  out.close();
  if (!out) {
    Logger::instance().error("failed to flush snapshot temp file: " + tmp);
    return false;
  }
  if (std::rename(tmp.c_str(), path_.c_str()) != 0) {
    Logger::instance().error("failed to replace snapshot file: " + path_);
    return false;
  }
  Logger::instance().debug("saved snapshot " + path_ + " (" +
                           std::to_string(metrics.size()) + " metrics)");
  return true;
}

bool SnapshotStore::start() {
  if (path_.empty()) {
    return true;
  }
  if (running_.exchange(true)) {
    return true;
  }
  thread_ = std::thread([this]() { runLoop(); });
  return true;
}

void SnapshotStore::stop() {
  if (!running_.exchange(false)) {
    if (thread_.joinable()) {
      thread_.join();
    }
    return;
  }
  if (thread_.joinable()) {
    thread_.join();
  }
  (void)save();
}

void SnapshotStore::runLoop() {
  using clock = std::chrono::steady_clock;
  auto next = clock::now() + std::chrono::seconds(interval_sec_);
  while (running_) {
    auto now = clock::now();
    if (now >= next) {
      (void)save();
      next = clock::now() + std::chrono::seconds(interval_sec_);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

}  // namespace simple_metricd
