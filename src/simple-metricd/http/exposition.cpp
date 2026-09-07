/**
 * @file exposition.cpp
 */

#include "simple-metricd/http/exposition.hpp"

#include "simple-metricd/metric/metric.hpp"
#include <algorithm>
#include <sstream>
#include <vector>

namespace simple_metricd {

std::string renderPrometheusText(const MetricRegistry &registry, bool include_metadata) {
  auto metrics = registry.list();
  std::sort(metrics.begin(), metrics.end(),
            [](const Metric *a, const Metric *b) { return a->name() < b->name(); });
  std::ostringstream out;
  for (const Metric *metric : metrics) {
    if (metric->type() == MetricType::Histogram) {
      const auto *hist = dynamic_cast<const HistogramMetric *>(metric);
      if (hist) {
        hist->writeExposition(out, include_metadata);
        continue;
      }
    }
    if (include_metadata) {
      const std::string help = metric->help();
      if (!help.empty()) {
        out << "# HELP " << metric->name() << " " << help << "\n";
      }
      out << "# TYPE " << metric->name() << " " << toString(metric->type()) << "\n";
    }
    out << metric->name();
    const std::string labels = metric->labels();
    if (!labels.empty()) {
      out << "{" << labels << "}";
    }
    out << " " << metric->value() << "\n";
  }
  return out.str();
}

}  // namespace simple_metricd
