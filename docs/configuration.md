# Configuration

Default listen port for labs is **19100**. Production templates may bind loopback or a management interface.

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
metric = NAME TYPE [VALUE] [help=...] [labels=k="v",...] [value=N]
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

See `config/templates/` for development, production, and high-security examples.
