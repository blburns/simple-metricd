# Technical debt

Tracked intentionally; none of these block the v0.1.0 skeleton.

- HTTP stack and scrape/push paths not present (milestones 3–7).
- Daemon stays in foreground; no POSIX daemonize or Windows service glue yet.
- Packaging scripts still carry SimpleDaemons boilerplate that will need metricd-specific install paths verified on each OS.
- No integration tests against a live `/metrics` endpoint until milestone 3.
- Privilege drop and rate limiting deferred to milestone 12.
