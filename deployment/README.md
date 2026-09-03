# simple-metricd Deployment

This directory contains deployment configurations and examples for simple-metricd.

## Directory Structure

```
deployment/
├── systemd/                    # Linux systemd service files
│   └── simple-metricd.service
├── launchd/                    # macOS launchd service files
│   └── com.simpledaemons.simple-metricd.plist
├── logrotate.d/                # Linux log rotation configuration
│   └── simple-metricd
├── windows/                    # Windows service management
│   └── simple-metricd.service
└── examples/                   # Deployment examples
    └── docker/                 # Docker deployment examples
        ├── docker-compose.yml
        └── README.md
```

## Platform-Specific Deployment

### Linux (systemd)

The unit starts `/usr/bin/simple-metricd --config /etc/simple-metricd/simple-metricd.conf --foreground` as user `simple-metricd` and creates `/var/lib/simple-metricd` and `/var/log/simple-metricd`.

1. **Install the service file** (if you did not install the package):
   ```bash
   sudo cp deployment/systemd/simple-metricd.service /etc/systemd/system/
   sudo systemctl daemon-reload
   ```

2. **Create user and directories:**
   ```bash
   sudo useradd --system --home-dir /var/lib/simple-metricd --no-create-home \
     --shell /usr/sbin/nologin simple-metricd
   sudo mkdir -p /etc/simple-metricd/tls /var/lib/simple-metricd /var/log/simple-metricd
   sudo chown root:simple-metricd /etc/simple-metricd /etc/simple-metricd/tls
   sudo chmod 0750 /etc/simple-metricd /etc/simple-metricd/tls
   sudo chown simple-metricd:simple-metricd /var/lib/simple-metricd /var/log/simple-metricd
   sudo chmod 0750 /var/lib/simple-metricd /var/log/simple-metricd
   ```

3. **Install config** (`640` `root:simple-metricd`), schemas, and TLS files. Copy `config/templates/production.conf` to `/etc/simple-metricd/simple-metricd.conf` and set `root_password`.

4. **Enable and start the service:**
   ```bash
   sudo systemctl enable simple-metricd
   sudo systemctl start simple-metricd
   ```

5. **Check status:**
   ```bash
   sudo systemctl status simple-metricd
   sudo journalctl -u simple-metricd -f
   ```

### macOS (launchd)

1. **Install the plist file:**
   ```bash
   sudo cp deployment/launchd/com.simpledaemons.simple-metricd.plist /Library/LaunchDaemons/
   sudo chown root:wheel /Library/LaunchDaemons/com.simpledaemons.simple-metricd.plist
   ```

2. **Load and start the service:**
   ```bash
   sudo launchctl load /Library/LaunchDaemons/com.simpledaemons.simple-metricd.plist
   sudo launchctl start com.simpledaemons.simple-metricd
   ```

3. **Check status:**
   ```bash
   sudo launchctl list | grep simple-metricd
   tail -f /var/log/simple-metricd/simple-metricd.out.log
   ```

### Windows

1. **Run as Administrator:**
   ```cmd
   # Install service
   deployment\windows\simple-metricd.service install
   
   # Start service
   deployment\windows\simple-metricd.service start
   
   # Check status
   deployment\windows\simple-metricd.service status
   ```

2. **Service management:**
   ```cmd
   # Stop service
   deployment\windows\simple-metricd.service stop
   
   # Restart service
   deployment\windows\simple-metricd.service restart
   
   # Uninstall service
   deployment\windows\simple-metricd.service uninstall
   ```

## Log Rotation (Linux)

1. **Install logrotate configuration:**
   ```bash
   sudo cp deployment/logrotate.d/simple-metricd /etc/logrotate.d/
   ```

2. **Test logrotate configuration:**
   ```bash
   sudo logrotate -d /etc/logrotate.d/simple-metricd
   ```

3. **Force log rotation:**
   ```bash
   sudo logrotate -f /etc/logrotate.d/simple-metricd
   ```

## Docker Deployment

See [examples/docker/README.md](examples/docker/README.md) for detailed Docker deployment instructions.

### Quick Start

```bash
# Build and run with Docker Compose
cd deployment/examples/docker
docker-compose up -d

# Check status
docker-compose ps
docker-compose logs simple-metricd
```

## Configuration

### Service Configuration

Each platform has specific configuration requirements:

- **Linux**: shipped unit is `/usr/bin/simple-metricd --config /etc/simple-metricd/simple-metricd.conf --foreground`
- **macOS**: plist `ProgramArguments` use `/usr/local/bin/simple-metricd` and `/etc/simple-metricd/simple-metricd.conf`
- **Windows**: `%PROGRAMFILES%\simple-metricd\simple-metricd.exe --config %PROGRAMDATA%\simple-metricd\simple-metricd.conf --foreground`

### Application Configuration

Place your application configuration in:
- **Linux/macOS**: `/etc/simple-metricd/simple-metricd.conf`
- **Windows**: `%PROGRAMDATA%\simple-metricd\simple-metricd.conf`

Data and logs:
- **Linux/macOS**: `/var/lib/simple-metricd`, `/var/log/simple-metricd`
- **Windows**: `%PROGRAMDATA%\simple-metricd\` and `%PROGRAMDATA%\simple-metricd\logs\`

## Security Considerations

### User and Permissions

1. **Create dedicated user:**
   ```bash
   # Linux
   sudo useradd --system --no-create-home --shell /bin/false simple-metricd
   
   # macOS
   sudo dscl . -create /Users/_simple-metricd UserShell /usr/bin/false
   sudo dscl . -create /Users/_simple-metricd UniqueID 200
   sudo dscl . -create /Users/_simple-metricd PrimaryGroupID 200
   sudo dscl . -create /Groups/_simple-metricd GroupID 200
   ```

2. **Set proper permissions:**
   ```bash
   # Configuration files
   sudo chown root:simple-metricd /etc/simple-metricd/simple-metricd.conf
   sudo chmod 640 /etc/simple-metricd/simple-metricd.conf
   
   # Log files
   sudo chown simple-metricd:simple-metricd /var/log/simple-metricd/
   sudo chmod 755 /var/log/simple-metricd/
   ```

### Firewall Configuration

Configure firewall rules as needed:

```bash
# Linux (ufw)
sudo ufw allow 389/tcp

# Linux (firewalld)
sudo firewall-cmd --permanent --add-port=389/tcp
sudo firewall-cmd --reload

# macOS
sudo pfctl -f /etc/pf.conf
```

## Monitoring

### Health Checks

1. **Service status:**
   ```bash
   # Linux
   sudo systemctl is-active simple-metricd
   
   # macOS
   sudo launchctl list | grep simple-metricd
   
   # Windows
   sc query simple-metricd
   ```

2. **Port availability:**
   ```bash
   netstat -tlnp | grep 389
   ss -tlnp | grep 389
   ```

3. **Process monitoring:**
   ```bash
   ps aux | grep simple-metricd
   top -p $(pgrep simple-metricd)
   ```

### Log Monitoring

1. **Real-time logs:**
   ```bash
   # Linux
   sudo journalctl -u simple-metricd -f
   
   # macOS
   tail -f /var/log/simple-metricd.log
   
   # Windows
   # Use Event Viewer or PowerShell Get-WinEvent
   ```

2. **Log analysis:**
   ```bash
   # Search for errors
   sudo journalctl -u simple-metricd --since "1 hour ago" | grep -i error
   
   # Count log entries
   sudo journalctl -u simple-metricd --since "1 day ago" | wc -l
   ```

## Troubleshooting

### Common Issues

1. **Service won't start:**
   - Check configuration file syntax
   - Verify user permissions
   - Check port availability
   - Review service logs

2. **Permission denied:**
   - Ensure service user exists
   - Check file permissions
   - Verify directory ownership

3. **Port already in use:**
   - Check what's using the port: `netstat -tlnp | grep 389`
   - Stop conflicting service or change port

4. **Service stops unexpectedly:**
   - Check application logs
   - Verify resource limits
   - Review system logs

### Debug Mode

Run the service in debug mode for troubleshooting:

```bash
# Linux/macOS
sudo -u simple-metricd /usr/bin/simple-metricd --config /etc/simple-metricd/simple-metricd.conf --foreground

# Windows
simple-metricd.exe --debug
```

### Log Levels

Adjust log level for more verbose output:

```bash
# Set log level in configuration
log_level = debug

# Or via environment variable
export SIMPLE_METRICD_LOG_LEVEL=debug
```

## Backup and Recovery

### Configuration Backup

```bash
# Backup configuration
sudo tar -czf simple-metricd-config-backup-$(date +%Y%m%d).tar.gz /etc/simple-metricd/

# Backup logs
sudo tar -czf simple-metricd-logs-backup-$(date +%Y%m%d).tar.gz /var/log/simple-metricd/
```

### Service Recovery

```bash
# Stop service
sudo systemctl stop simple-metricd

# Restore configuration
sudo tar -xzf simple-metricd-config-backup-YYYYMMDD.tar.gz -C /

# Start service
sudo systemctl start simple-metricd
```

## Updates

### Service Update Process

1. **Stop service:**
   ```bash
   sudo systemctl stop simple-metricd
   ```

2. **Backup current version:**
   ```bash
   sudo cp /usr/bin/simple-metricd /usr/bin/simple-metricd.backup
   ```

3. **Install new version:**
   ```bash
   sudo cp simple-metricd /usr/bin/
   sudo chmod +x /usr/bin/simple-metricd
   ```

4. **Start service:**
   ```bash
   sudo systemctl start simple-metricd
   ```

5. **Verify update:**
   ```bash
   sudo systemctl status simple-metricd
   simple-metricd --version
   ```
