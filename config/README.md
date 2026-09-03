# Configuration files for simple-metricd

Use `config/templates/` for shipped defaults and `config/examples/` for documented samples.

Key/value syntax (`name = value`). Comments start with `#`.

| Key | Default | Notes |
|-----|---------|-------|
| listen_address | 0.0.0.0 | Status-endpoint bind address (used from v0.4.0) |
| listen_port | 8080 | Status HTTP port (use 19100 in development) |
| enable_https | false | Require cert/key when true (v0.5.0) |
| tls_cert_file | | Server certificate |
| tls_key_file | | Server private key |
| tls_ca_file | | Optional CA |
| check_interval | 30 | Seconds between scheduled runs (v0.2.0) |
| check_timeout | 5 | Per-check timeout in seconds |
| result_store | memory | `memory` or `sqlite` (sqlite is v0.8.0) |
| sqlite_file | | Required when `result_store = sqlite` |
| log_file | | Optional log path |
| log_level | info | `debug`, `info`, `warning`, `error`, `fatal` |
| foreground | true | Stay in the foreground |
| check | (none) | Repeatable. `name type target [expect=…]` |

## Check line

```
check = name type target
check = name type target expect=200
```

| Type | Target example | Milestone |
|------|----------------|-----------|
| `tcp` | `127.0.0.1:22` | 2 |
| `http` | `http://127.0.0.1:8080/healthz` | 3 |
| `https` | `https://example.com/healthz` | 5 |
| `process` | `simple-httpd` | 9 |
| `icmp` | `192.0.2.1` | 9 |
| `script` | `/usr/local/bin/check-disk.sh` | 9 |

In v0.1.0 the lines are parsed and listed by `metricctl list`. `run()` does not probe yet.
