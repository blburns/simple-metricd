# simple-metricd

Lightweight Prometheus-style metrics daemon for SimpleDaemons hosts: expose and collect text metrics locally, scrape configured targets, accept push updates, and optionally snapshot values. Not a full TSDB or PromQL engine.

**Current version:** 1.0.0

## Status

Production-usable single-host metrics daemon. Counter/gauge registry, HTTP `/metrics`, pull scrape, push ingest, optional snapshots, TLS/ACL, rate limit, and privilege drop. See [ROADMAP.md](ROADMAP.md) and [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md).

## Quick start

```sh
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simple-metricd --config config/templates/development.conf --foreground
curl -s http://127.0.0.1:19100/metrics
./build/metricctl scrape --config config/templates/development.conf
```

## Docs

- [docs/configuration.md](docs/configuration.md)
- [docs/operations.md](docs/operations.md)
- [docs/security.md](docs/security.md)

## License

Apache-2.0
