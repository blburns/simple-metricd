# Operations

## Run under a supervisor

Prefer systemd, launchd, or a Windows service. The daemon stays in the foreground (`--foreground`). `--daemon` does not fork.

```sh
simple-metricd --config /etc/simple-metricd/simple-metricd.conf --foreground
```

## Health and scrape

- `GET /healthz` → `ok`
- `GET /status` → thin JSON (`version`, `metrics`, `port`)
- `GET /metrics` → Prometheus text 0.0.4
- `metricctl scrape --config ...` → fetch live `/metrics`

## Directories

Package installs should create `/var/lib/simple-metricd` and `/var/log/simple-metricd` (or platform equivalents) owned by the service user. The systemd unit uses `StateDirectory` / `LogsDirectory`.

## Reload / stop

Use the OS supervisor to restart. In-process `stop` / `reload` subcommands are not part of the 1.0 contract.

## Limits

Not a TSDB. No PromQL. No multi-node federation. Snapshots are lightweight, not long-term retention.
