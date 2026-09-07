# Configuration files for simple-metricd

Key/value syntax (`name = value`). Comments start with `#`.
Default lab port is **19100** (unprivileged).

| Path | Purpose |
|------|---------|
| `templates/` | Shipped defaults for development, production, and high-security installs |
| `examples/` | Documented usage samples (copy and edit) |

Full reference: [docs/configuration.md](../docs/configuration.md).

## Quick start

```sh
# Lab: listen on loopback, list metrics, scrape live exposition
simple-metricd --config config/templates/development.conf --foreground
metricctl list --config config/templates/development.conf
metricctl scrape --config config/templates/development.conf
curl -s http://127.0.0.1:19100/metrics
curl -s http://127.0.0.1:19100/healthz
```

## Keys (1.0.0)

### Core

| Key | Default | Notes |
|-----|---------|-------|
| `listen_address` | `0.0.0.0` | Bind for `/metrics`, `/healthz`, `/status`, `/ingest` |
| `listen_port` | `19100` | Unprivileged lab port |
| `log_level` | `info` | `debug`, `info`, `warning`, `error`, `fatal` |
| `log_file` | (none) | Optional file sink |
| `foreground` | `true` | Stay in foreground (use with systemd/launchd) |

### Static metrics

```
metric = NAME TYPE [VALUE] [help=...] [labels=k="v",...] [value=N] [buckets=...]
```

| Type | Notes |
|------|-------|
| `counter` | Non-decreasing |
| `gauge` | Any finite value |
| `histogram` | Requires `buckets=` (comma-separated upper bounds) |

### Collect

| Key | Example |
|-----|---------|
| `scrape_target` | `127.0.0.1:9100/metrics interval=15 timeout=5` |
| `snapshot_file` | `/var/lib/simple-metricd/registry.snapshot` |
| `snapshot_interval` | `60` |

### Security

| Key | Notes |
|-----|-------|
| `tls_cert_file` / `tls_key_file` / `tls_ca_file` | HTTPS when built with `ENABLE_SSL=ON` |
| `allow_ip` / `deny_ip` | Comma-separated hosts or `prefix*` |
| `auth_user` / `auth_password` | Optional basic auth (`Authorization: Basic user:pass`) |
| `public_healthz` | Keep `/healthz` open when auth is set |
| `rate_limit_per_minute` | Per-peer API limit (`0` = off) |
| `run_as_user` | Drop privileges after bind |

## Example catalogue

| Example | What it shows |
|---------|----------------|
| [examples/simple/](examples/simple/) | Minimal local `/metrics` |
| [examples/labels/](examples/labels/) | HELP/TYPE, labels, counters/gauges |
| [examples/histograms/](examples/histograms/) | Histogram buckets |
| [examples/scrape/](examples/scrape/) | Pull scrape of remote `/metrics` |
| [examples/push-ingest/](examples/push-ingest/) | Ready for `POST /ingest` |
| [examples/snapshot/](examples/snapshot/) | File snapshot load/save |
| [examples/lab-stack/](examples/lab-stack/) | Scrape healthd + httpd in a lab |
| [examples/security/](examples/security/) | ACL, auth, rate limit, TLS hooks |
| [examples/production/](examples/production/) | Service-user host install |
| [examples/advanced/](examples/advanced/) | Full feature kitchen sink |

## Push ingest (quick)

With a running daemon:

```sh
curl -s -X POST http://127.0.0.1:19100/ingest \
  --data-binary $'demo_requests_total counter 3\ndemo_temp_celsius gauge 21.5\n'
```

Or Prometheus sample lines / `# TYPE` comments. Alias path: `/api/v1/import`.
