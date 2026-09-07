# simple-metricd — Roadmap Checklist

**Current version:** 0.2.0  
**Overall progress:** Milestones 1–2 in tree; milestones 3–12 planned  
**Honest assessment:** Prefer [PROGRESS_REPORT.md](PROGRESS_REPORT.md). Public plan: [ROADMAP.md](../ROADMAP.md). Versions: [VERSIONING.md](../VERSIONING.md).

**Product line:** Production (Apache 2.0) — single-host metrics daemon (not Prometheus, not a TSDB).

---

## Milestone 1 — Skeleton — v0.1.0

**Status:** ✅ Released (`v0.1.0`)

### Build & packaging
- [x] Cross-platform CMake / GNU Make build
- [x] Packaging for Linux, macOS, Windows, FreeBSD
- [x] Daemon and `metricctl` CLI entry points
- [x] Pluggable metric interface (counter + gauge stubs)
- [x] Config templates and example metric lines

---

## Milestone 2 — Registry — v0.2.0

**Status:** ✅ Released (`v0.2.0`)

- [x] In-memory counter and gauge registry
- [x] Register and update metric values
- [x] `metricctl list` showing metrics

---

## Milestone 3 — Exposition — v0.3.0

**Status:** ⏳ Planned

- [ ] HTTP `/metrics` (Prometheus text 0.0.4)
- [ ] Lab listen on 19100
- [ ] `metricctl scrape` against a live daemon

---

## Milestone 4 — Metadata — v0.4.0

**Status:** ⏳ Planned

- [ ] HELP and TYPE lines
- [ ] Label sets on metric families
- [ ] Config-defined static metrics

---

## Milestone 5 — TLS — v0.5.0

**Status:** ⏳ Planned

- [ ] HTTPS `/metrics`
- [ ] TLS for the metrics listener
- [ ] Certificate and CA configuration

---

## Milestone 6 — Pull scrape — v0.6.0

**Status:** ⏳ Planned

- [ ] Interval scheduler for scrape targets
- [ ] Per-target timeout
- [ ] Merge scraped series into registry

---

## Milestone 7 — Push ingest — v0.7.0

**Status:** ⏳ Planned

- [ ] HTTP push endpoint for counter/gauge updates
- [ ] Structured log hook on ingest errors

---

## Milestone 8 — Persistence — v0.8.0

**Status:** ⏳ Planned

- [ ] SQLite or file snapshot of recent values
- [ ] Memory registry remains for labs

---

## Milestone 9 — Aggregation — v0.9.0

**Status:** ⏳ Planned

- [ ] Rate and window statistics
- [ ] Basic histogram buckets

---

## Milestone 10 — Concurrent scraper — v0.10.0

**Status:** ⏳ Planned

- [ ] Parallel scrape workers
- [ ] Per-target timeout unchanged
- [ ] Fair scheduling

---

## Milestone 11 — Access control — v0.11.0

**Status:** ⏳ Planned

- [ ] Auth on the metrics endpoint
- [ ] IP allow / deny
- [ ] Anonymous `/metrics` can stay public in the lab template

---

## Milestone 12 — Hardening — v0.12.0

**Status:** ⏳ Planned

- [ ] Apply `log_level`
- [ ] Rate limit on the metrics API
- [ ] Privilege drop / service-user defaults
- [ ] Optional install prefix

---

*Last updated: September 2026*
