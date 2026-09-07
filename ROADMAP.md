# simple-metricd roadmap

**Honesty note:** Prefer [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md) when docs disagree. Item-level checklists: [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md). Versions: [VERSIONING.md](VERSIONING.md). Overview: [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md).

## Milestone 1 — Skeleton — v0.1.0

- Cross-platform CMake / GNU Make build
- Packaging for Linux, macOS, Windows, FreeBSD
- Daemon and `metricctl` CLI entry points
- Pluggable metric interface (counter + gauge stubs)
- Config templates and example metric lines

## Milestone 2 — Registry — v0.2.0

- [x] In-memory counter and gauge registry
- [x] Register and update metric values
- [x] `metricctl list` showing configured and registered metrics

## Milestone 3 — Exposition — v0.3.0

- HTTP `/metrics` (Prometheus text format 0.0.4)
- Lab listen on an unprivileged port (19100)
- `metricctl scrape` against a live daemon

## Milestone 4 — Metadata — v0.4.0

- HELP and TYPE lines in exposition output
- Label sets on metric families
- Config-defined static metrics

## Milestone 5 — TLS — v0.5.0

- HTTPS `/metrics`
- TLS for the metrics listener
- Certificate and CA configuration

## Milestone 6 — Pull scrape — v0.6.0

- Interval scheduler scraping remote `/metrics` targets
- Per-target configuration and timeout
- Merge scraped series into the local registry

## Milestone 7 — Push ingest — v0.7.0

- HTTP push endpoint for counter/gauge updates
- Structured log hook on ingest errors

## Milestone 8 — Persistence — v0.8.0

- SQLite or file snapshot of recent values (not a TSDB)
- Memory registry remains for labs

## Milestone 9 — Aggregation — v0.9.0

- Simple rate and window statistics over recent samples
- Basic histogram buckets

## Milestone 10 — Concurrent scraper — v0.10.0

- Run independent scrape targets in parallel
- Per-target timeout unchanged
- Fair scheduling so a slow target does not starve others

## Milestone 11 — Access control — v0.11.0

- Auth on the metrics HTTP endpoint
- IP allow / deny
- Anonymous `/metrics` can stay public in the lab template

## Milestone 12 — Hardening — v0.12.0

- Apply `log_level` (parsed today, unused until then)
- Rate limit on the metrics API
- Privilege drop / service-user defaults
- Optional install prefix so binary names stay `simple-metricd` / `metricctl`

## 1.0.0 cut — production-usable single-host metrics daemon

After milestone **0.12.0**. **1.0.0** is a hygiene and contract cut on top of that series, not a new metric type.

### Contract (write this into README / docs / CHANGELOG at tag time)

- [x] Counter/gauge registry; `/metrics` exposition; pull scrape; push ingest; optional persistence; `log_level`; metrics-API rate limit
- [x] One process, one host; run under systemd / launchd / a Windows service (`--daemon` does not fork)
- [x] Known limits that stay: no full TSDB; no PromQL; no long-term retention; no multi-node federation
- [x] Drop “early development” / “skeleton” language; refresh the `simple-metricd` blurb in SimpleDaemons `docs/FUTURE_DAEMONS.md`

### Packaging (must match production templates)

- [x] Create `/var/lib/simple-metricd` and `/var/log/simple-metricd` (or platform equivalents) with the service user
- [x] systemd / launchd / Windows unit: `ExecStart` path and `--foreground --config` must match the installed binary and `/etc/simple-metricd/simple-metricd.conf`

### Optional polish (do not block 1.0 unless “safe on a public metrics port” is the bar)

- [x] Max concurrent outbound scrapes
- [ ] Idle timeout on the metrics listener
- [x] A CI workflow that builds and runs `ctest`

### Do not pull into 1.0

Items under **Later** and **Out of scope**. A 1.1+ can add them without retconning 1.0.

## Later (not scheduled)

- Prometheus remote-write to external systems
- Multi-node / federated metric mesh
- Full Pushgateway compatibility
- `--daemon` fork and `stop` / `status` / `reload` (use the OS supervisor)

## Out of scope (1.0)

- A full time-series database
- PromQL query engine
- Replacing Prometheus, Grafana, or a SaaS APM
- Long-term metric retention beyond lightweight snapshots
