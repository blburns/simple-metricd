Name:           simple-metricd
Version:        ${VERSION}
Release:        1%{?dist}
Summary:        Lightweight health check daemon
License:        Apache-2.0
URL:            https://github.com/SimpleDaemons/%{name}
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  openssl-devel
BuildRequires:  pkgconfig
Requires:       openssl-libs

%description
simple-metricd is a lightweight health check daemon. It watches
configured HTTP/TCP endpoints and aggregates results. v0.1.0 is
the skeleton. Optional -DMETRIC_CLI_PREFIX=simple- prefixes metricctl.

%prep
%setup -q

%build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_TESTS=OFF
make %{?_smp_mflags}

%install
cd build
make DESTDIR=%{buildroot} install
mkdir -p %{buildroot}%{_localstatedir}/lib/%{name}
mkdir -p %{buildroot}%{_localstatedir}/log/%{name}
mkdir -p %{buildroot}%{_sysconfdir}/%{name}/tls

%pre
if ! getent group simple-metricd >/dev/null 2>&1; then
    groupadd -r simple-metricd
fi
if ! getent passwd simple-metricd >/dev/null 2>&1; then
    useradd -r -M -d %{_localstatedir}/lib/%{name} -s /sbin/nologin \
        -g simple-metricd -c "%{name} service user" simple-metricd
fi

%post
if [ -f %{_sysconfdir}/%{name}/%{name}.conf ]; then
    chown root:simple-metricd %{_sysconfdir}/%{name}/%{name}.conf
    chmod 0640 %{_sysconfdir}/%{name}/%{name}.conf
elif [ -f %{_sysconfdir}/%{name}/templates/production.conf ]; then
    cp %{_sysconfdir}/%{name}/templates/production.conf \
        %{_sysconfdir}/%{name}/%{name}.conf
    chown root:simple-metricd %{_sysconfdir}/%{name}/%{name}.conf
    chmod 0640 %{_sysconfdir}/%{name}/%{name}.conf
fi
chown root:simple-metricd %{_sysconfdir}/%{name} %{_sysconfdir}/%{name}/tls 2>/dev/null || true
chmod 0750 %{_sysconfdir}/%{name} %{_sysconfdir}/%{name}/tls 2>/dev/null || true
chown simple-metricd:simple-metricd %{_localstatedir}/lib/%{name} %{_localstatedir}/log/%{name}
chmod 0750 %{_localstatedir}/lib/%{name} %{_localstatedir}/log/%{name}
if command -v systemd-tmpfiles >/dev/null 2>&1; then
    systemd-tmpfiles --create %{_prefix}/lib/tmpfiles.d/%{name}.conf >/dev/null 2>&1 || true
fi
if command -v systemctl >/dev/null 2>&1; then
    systemctl daemon-reload >/dev/null 2>&1 || true
fi

%preun
if [ "$1" -eq 0 ] && command -v systemctl >/dev/null 2>&1; then
    systemctl stop %{name} >/dev/null 2>&1 || true
    systemctl disable %{name} >/dev/null 2>&1 || true
fi

%postun
if command -v systemctl >/dev/null 2>&1; then
    systemctl daemon-reload >/dev/null 2>&1 || true
fi

%files
%license LICENSE
%doc README.md
%{_bindir}/%{name}
%{_bindir}/metricctl
%{_includedir}/simple-metricd/
%{_unitdir}/%{name}.service
%{_prefix}/lib/sysusers.d/%{name}.conf
%{_prefix}/lib/tmpfiles.d/%{name}.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/%{name}
%dir %attr(0750,root,simple-metricd) %{_sysconfdir}/%{name}
%dir %attr(0750,root,simple-metricd) %{_sysconfdir}/%{name}/tls
%dir %attr(0750,simple-metricd,simple-metricd) %{_localstatedir}/lib/%{name}
%dir %attr(0750,simple-metricd,simple-metricd) %{_localstatedir}/log/%{name}
%{_sysconfdir}/%{name}/templates/
%{_sysconfdir}/%{name}/examples/
%ghost %config(noreplace) %{_sysconfdir}/%{name}/%{name}.conf

%changelog
* Mon Aug 24 2026 SimpleDaemons <info@simpledaemons.com> - ${VERSION}-1
- Align packaging with production paths and systemd unit
