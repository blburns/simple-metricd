# Progress report

**v0.2.0** completes Milestone 2 (Registry): in-memory counter/gauge registry, mutable values, daemon registration from config, and `metricctl list` with live values. See [VERSIONING.md](../VERSIONING.md).

What works today: load/validate config; register counters/gauges; update values (counters non-decreasing); list registered metrics with values; daemon foreground loop; `--version` / `--test-config`.

What does not work: HTTP `/metrics`, scrape, push, TLS, persistence, ACLs, privilege drop, `--daemon` fork, `stop` / `reload` subcommands.
