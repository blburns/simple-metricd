# Project overview — simple-metricd

**simple-metricd** is a single-host metrics daemon: expose and collect Prometheus text metrics locally, scrape configured targets, accept push updates, and optionally forward upstream. It integrates with [simple-healthd](../simple-healthd) (health) and future log daemons for operational clarity.

## Goals

- Small, packagable C++ daemon with `metricctl`
- Prometheus text exposition (`/metrics`)
- Pull scrape and push ingest
- Optional short-lived snapshot persistence (not a TSDB)
- Cross-platform packaging (Linux, macOS, Windows, FreeBSD)

## Non-goals (1.0)

- Full time-series database
- PromQL query engine
- Long-term retention or replacing Prometheus/Grafana
- Multi-node federation

## Current state

**v0.1.0** delivers the skeleton: CMake/Make, packaging tree, config parser with `metric =` lines, counter/gauge stubs, daemon foreground loop, and `metricctl list` / `--version` / `--test-config`.

## Related docs

- [ROADMAP.md](ROADMAP.md) — milestones 1–12 and 1.0.0 contract
- [VERSIONING.md](VERSIONING.md) — version ↔ milestone map
- [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md) — honesty report
