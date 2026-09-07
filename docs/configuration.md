# Configuration

Default listen port for labs is **19100**. Production templates may bind loopback or a management interface.

Shipped files live under [`config/`](../config/README.md):

- `config/templates/` — development / production / high-security defaults
- `config/examples/` — usage samples (simple, labels, histograms, scrape, push-ingest, snapshot, lab-stack, security, production, advanced)

## Core

| Key | Default | Notes |
|-----|---------|-------|
| `listen_address` | `0.0.0.0` | Bind address for `/metrics` |
| `listen_port` | `19100` | Unprivileged lab port |
| `log_level` | `info` | `debug`, `info`, `warning`, `error`, `fatal` |
| `log_file` | (none) | Optional file sink |
| `foreground` | `true` | Supervisor-friendly; `--daemon` does not fork |

## Metrics

```
metric = NAME TYPE [VALUE] [help=...] [labels=k="v",...] [value=N] [buckets=...]
```

Types: `counter`, `gauge`, `histogram` (with `buckets=`).

## Security

| Key | Notes |
|-----|-------|
| `tls_cert_file` / `tls_key_file` / `tls_ca_file` | HTTPS when built with `ENABLE_SSL=ON` |
| `allow_ip` / `deny_ip` | Comma-separated prefixes (`10.0.0.*`) |
| `auth_user` / `auth_password` | Optional basic auth (`Authorization: Basic user:pass`) |
| `public_healthz` | Keep `/healthz` open when auth is set |
| `rate_limit_per_minute` | Per-client API limit (`0` disables) |
| `run_as_user` | Drop privileges after bind |

## Collect

| Key | Notes |
|-----|-------|
| `scrape_target` | `host:port/path interval=15 timeout=5` |
| `snapshot_file` | Registry snapshot path |
| `snapshot_interval` | Seconds between saves |

## Common recipes

**Local scrape into Prometheus**

```yaml
scrape_configs:
  - job_name: simple-metricd
    static_configs:
      - targets: ["127.0.0.1:19100"]
```

**Push a counter**

```sh
curl -s -X POST http://127.0.0.1:19100/ingest \
  --data-binary $'app_requests_total counter 9\n'
```

**Aggregate node_exporter**

```
scrape_target = 127.0.0.1:9100/metrics interval=15 timeout=5
```

See `config/examples/` for full files.
