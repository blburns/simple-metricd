# Security

## Default posture

- Lab template may expose anonymous `/metrics` on loopback (`127.0.0.1:19100`).
- High-security template sets `allow_ip`, optional auth, rate limits, and `run_as_user`.

## TLS

Build with `-DENABLE_SSL=ON` (CMake default from 0.12.0). Configure:

```
tls_cert_file = /etc/simple-metricd/certs/server.crt
tls_key_file = /etc/simple-metricd/certs/server.key
tls_ca_file = /etc/simple-metricd/certs/ca.crt
```

## ACL and auth

Deny rules win. When `allow_ip` is set, clients must match. Optional basic auth gates `/metrics` and ingest; `/healthz` can stay public via `public_healthz = true`.

## Rate limit

`rate_limit_per_minute` applies per peer. Exceeded clients receive HTTP 429.

## Privilege drop

After binding the listen port, the daemon drops to `run_as_user` when set. systemd units should still run as the service user for defense in depth.
