# Versioning

simple-metricd uses [Semantic Versioning](https://semver.org/) on the **0.x** series. Each completed [roadmap](ROADMAP.md) milestone is a **minor** bump. Fixes and docs inside a milestone are **patch** bumps. **1.0.0** is a hygiene/contract cut after milestone 12.

Git tags and GitHub Releases use the `vMAJOR.MINOR.PATCH` form and point at the commit that finished that version.

## Milestone map

| Version | Milestone | What it means | Status |
|---------|-----------|---------------|--------|
| **0.1.0** | 1 — Skeleton | Build, packaging, CLI, config, metric stubs | Released (`v0.1.0`) |
| **0.2.0** | 2 — Registry | In-memory counter/gauge registry | Released (`v0.2.0`) |
| **0.3.0** | 3 — Exposition | HTTP `/metrics` (Prometheus text 0.0.4) | Released (`v0.3.0`) |
| **0.4.0** | 4 — Metadata | HELP/TYPE, labels, static metrics | Planned |
| **0.5.0** | 5 — TLS | HTTPS `/metrics` | Planned |
| **0.6.0** | 6 — Pull scrape | Interval scrape of remote targets | Planned |
| **0.7.0** | 7 — Push ingest | HTTP push for counter/gauge updates | Planned |
| **0.8.0** | 8 — Persistence | SQLite/file snapshot (not TSDB) | Planned |
| **0.9.0** | 9 — Aggregation | Rate/window stats; basic histograms | Planned |
| **0.10.0** | 10 — Concurrency | Parallel scrape workers | Planned |
| **0.11.0** | 11 — Access control | Auth + IP allow/deny | Planned |
| **0.12.0** | 12 — Hardening | log_level, rate limit, privilege drop | Planned |
| **1.0.0** | Contract cut | Docs/packaging promise after 0.12.0 | Planned |

## Rules

- Do not retcon a released tag. If 0.1.0 already shipped, a skeleton bug fix is **0.1.1**, not another 0.1.0.
- `include/simple-metricd/version.hpp` (`kVersion`) and `CMakeLists.txt` (`project(... VERSION ...)`) stay in lockstep with the tag.
- `CHANGELOG.md` has a section per released version. Work toward the next milestone lives under **Unreleased** until that tag is cut.
- Cutting a release: update version files and changelog, commit, `git tag -a vX.Y.Z`, push the tag, create the GitHub Release from that tag.
