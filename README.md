# simple-metricd

Lightweight Prometheus-style metrics daemon for SimpleDaemons hosts: expose and collect text metrics locally, scrape configured targets, accept push updates, and optionally forward upstream. Not a full TSDB or PromQL engine.

**Current version:** 0.3.0 (Milestone 3 — Exposition)

## Status

Early development. v0.3.0 serves HTTP `/metrics`, `/healthz`, and `/status`, and `metricctl scrape` reads live exposition. TLS, pull scrape, push, and persistence arrive in later milestones. See [ROADMAP.md](ROADMAP.md) and [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md).

## Quick start

```sh
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure

./build/simple-metricd --version
./build/simple-metricd --test-config --config config/templates/development.conf
./build/metricctl --config config/templates/development.conf list
```

Foreground loop (no HTTP yet):

```sh
./build/simple-metricd --config config/templates/development.conf --foreground
```

Default lab listen port is **19100** (configured; exposition is milestone 3).

## Documentation

| Doc | Purpose |
|-----|---------|
| [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) | Product scope and non-goals |
| [ROADMAP.md](ROADMAP.md) | Public milestone plan |
| [VERSIONING.md](VERSIONING.md) | SemVer + milestone map |
| [CHANGELOG.md](CHANGELOG.md) | Release notes |
| [RELEASING.md](RELEASING.md) | How to cut a release |
| [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md) | Item-level checklist |
| [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md) | What works today |

## License

Apache-2.0. See [LICENSE](LICENSE).
