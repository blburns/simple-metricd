# Progress report

**v0.1.0** completes Milestone 1 (Skeleton): CMake/Make build, packaging tree, config parser, metric stubs, `simple-metricd` + `metricctl`, and config unit tests. See [VERSIONING.md](../VERSIONING.md).

What works today: load/validate config; list configured metrics; daemon foreground loop with stub metrics; `--version` / `--test-config`.

What does not work: HTTP `/metrics`, scrape, push, TLS, persistence, ACLs, privilege drop, `--daemon` fork, `stop` / `reload` subcommands.
