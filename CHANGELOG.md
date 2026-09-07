# Changelog

All notable changes to simple-metricd are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/) (see [VERSIONING.md](VERSIONING.md)).

## [Unreleased]

### Added
- (none yet)

## [0.5.0] — 2026-09-07

### Added
- Optional TLS certificate/key/CA for HTTPS /metrics
- IP allow/deny lists and optional basic auth on the metrics API
- High-security config template defaults for loopback ACL

## [0.4.0] — 2026-09-07

### Added
- Config `metric` lines accept `help=`, `labels=`, and `value=` tokens
- Static metrics from config expose HELP/TYPE and label sets on `/metrics`

## [0.3.0] — 2026-09-07

### Added
- TCP listener and HTTP metrics server on `listen_address`/`listen_port`
- `GET /metrics` Prometheus text exposition (0.0.4) from the registry
- `GET /healthz` and thin JSON `GET /status`
- `metricctl scrape` to fetch live `/metrics` from a running daemon
- HTTP smoke tests for exposition and endpoints

## [0.2.0] — 2026-09-07

### Added
- In-memory `MetricRegistry` for counter and gauge registration
- Mutable counter (non-decreasing) and gauge metric implementations
- Daemon loads configured metrics into the registry on initialize
- `metricctl list` prints registered name, type, and live value
- Unit tests for registry register/update semantics

## [0.1.0] — 2026-09-02

### Added
- Cross-platform CMake / GNU Make skeleton and packaging tree
- `simple-metricd` daemon and `metricctl` CLI entry points
- Config parser with `listen_*`, `log_level`, and `metric =` lines
- Counter/gauge metric interface stubs (`makeMetric`)
- `metricctl list`, `--version`, and `--test-config`
- Unit tests for config load and validation
- Roadmap, versioning, and project documentation for milestones 1–12
