# Progress report

**v0.4.0** completes Milestone 4 (Metadata): HELP/TYPE lines, label sets, and config-defined static metrics with `help=` / `labels=` / `value=`. See [VERSIONING.md](../VERSIONING.md).

What works today: registry; HTTP `/metrics` with HELP/TYPE/labels; `/healthz` `/status`; `metricctl list` / `scrape`; config static metrics.

What does not work: TLS, pull scrape scheduler, push ingest, persistence, ACLs, privilege drop, `--daemon` fork.
