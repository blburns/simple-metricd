# Progress report

**v0.3.0** completes Milestone 3 (Exposition): HTTP `/metrics` (Prometheus text 0.0.4), `/healthz` and thin `/status`, lab listen on 19100, and `metricctl scrape`. See [VERSIONING.md](../VERSIONING.md).

What works today: load/validate config; register counters/gauges; update values; list metrics; bind HTTP and scrape live exposition; foreground loop; `--version` / `--test-config`.

What does not work: config label parsing polish (Milestone 4), TLS, push, scrape scheduler, persistence, ACLs, privilege drop, `--daemon` fork, `stop` / `reload` subcommands.
