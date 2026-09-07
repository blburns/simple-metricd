/**
 * @file net.cpp
 * @brief TCP and minimal HTTP helpers (plain sockets; TLS hooks for later milestones)
 */

#include "simple-metricd/utils/net.hpp"
#include "simple-metricd/security/tls.hpp"

#ifndef SIMPLE_METRICD_WINDOWS
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <ws2tcpip.h>
#endif

#include <cerrno>
#include <cstring>
#include <sstream>

namespace simple_metricd {

namespace {

bool waitReadable(socket_t fd, int timeout_ms) {
#ifdef SIMPLE_METRICD_WINDOWS
  WSAPOLLFD pfd{};
  pfd.fd = fd;
  pfd.events = POLLIN;
  return WSAPoll(&pfd, 1, timeout_ms) > 0;
#else
  pollfd pfd{};
  pfd.fd = fd;
  pfd.events = POLLIN;
  return ::poll(&pfd, 1, timeout_ms) > 0;
#endif
}

bool sendBytes(socket_t fd, const char *data, size_t len) {
  size_t sent = 0;
  while (sent < len) {
#ifdef SIMPLE_METRICD_WINDOWS
    const int n = ::send(fd, data + sent, static_cast<int>(len - sent), 0);
#else
    const ssize_t n = ::send(fd, data + sent, len - sent, 0);
#endif
    if (n <= 0) {
      return false;
    }
    sent += static_cast<size_t>(n);
  }
  return true;
}

}  // namespace

bool initializeSockets() {
#ifdef SIMPLE_METRICD_WINDOWS
  WSADATA data;
  return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
  return true;
#endif
}

void shutdownSockets() {
#ifdef SIMPLE_METRICD_WINDOWS
  WSACleanup();
#endif
}

bool parseHostPort(const std::string &target, std::string &host, port_t &port,
                   port_t default_port) {
  if (target.empty()) {
    return false;
  }
  const auto colon = target.rfind(':');
  if (colon != std::string::npos && target.find(':') == colon) {
    host = target.substr(0, colon);
    port = static_cast<port_t>(std::stoi(target.substr(colon + 1)));
    return !host.empty();
  }
  host = target;
  port = default_port;
  return true;
}

TcpConnection::TcpConnection(socket_t fd, std::string peer)
    : fd_(fd), peer_(std::move(peer)) {}

TcpConnection::TcpConnection(TcpConnection &&other) noexcept
    : fd_(other.fd_), peer_(std::move(other.peer_)), ssl_(other.ssl_) {
  other.fd_ = INVALID_SOCKET_VALUE;
  other.ssl_ = nullptr;
}

TcpConnection &TcpConnection::operator=(TcpConnection &&other) noexcept {
  if (this != &other) {
    close();
    fd_ = other.fd_;
    peer_ = std::move(other.peer_);
    ssl_ = other.ssl_;
    other.fd_ = INVALID_SOCKET_VALUE;
    other.ssl_ = nullptr;
  }
  return *this;
}

TcpConnection::~TcpConnection() { close(); }

bool TcpConnection::valid() const { return fd_ != INVALID_SOCKET_VALUE; }

void TcpConnection::close() {
  if (fd_ != INVALID_SOCKET_VALUE) {
    CLOSE_SOCKET(fd_);
    fd_ = INVALID_SOCKET_VALUE;
  }
  ssl_ = nullptr;
}

bool TcpConnection::handshakeTls(const TlsContext &, bool) {
  return false;
}

bool TcpConnection::sendAll(const std::string &data) {
  return sendBytes(fd_, data.data(), data.size());
}

bool TcpConnection::recvLine(std::string &line, int timeout_ms) {
  line.clear();
  char ch = 0;
  while (line.size() < 8192) {
    if (!waitReadable(fd_, timeout_ms)) {
      return false;
    }
    const int n = static_cast<int>(::recv(fd_, &ch, 1, 0));
    if (n <= 0) {
      return false;
    }
    if (ch == '\n') {
      while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
      }
      return true;
    }
    line.push_back(ch);
  }
  return false;
}

bool TcpConnection::recvAvailable(std::string &data, int timeout_ms, std::size_t max_bytes) {
  if (!waitReadable(fd_, timeout_ms)) {
    return false;
  }
  char buf[4096];
  const int n = static_cast<int>(::recv(fd_, buf, sizeof(buf), 0));
  if (n <= 0) {
    return false;
  }
  data.append(buf, static_cast<size_t>(n));
  return data.size() <= max_bytes;
}

TcpListener::~TcpListener() { close(); }

bool TcpListener::bindAndListen(const std::string &address, port_t port) {
  close();
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  addrinfo *res = nullptr;
  const auto port_str = std::to_string(port);
  if (getaddrinfo(address.empty() ? nullptr : address.c_str(), port_str.c_str(), &hints,
                  &res) != 0 ||
      !res) {
    return false;
  }
  for (addrinfo *p = res; p; p = p->ai_next) {
    fd_ = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (fd_ == INVALID_SOCKET_VALUE) {
      continue;
    }
    int yes = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes), sizeof(yes));
    if (::bind(fd_, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0 &&
        ::listen(fd_, 64) == 0) {
      sockaddr_storage addr{};
      socklen_t len = sizeof(addr);
      if (getsockname(fd_, reinterpret_cast<::sockaddr *>(&addr), &len) == 0) {
        if (addr.ss_family == AF_INET) {
          bound_port_ = ntohs(reinterpret_cast<sockaddr_in *>(&addr)->sin_port);
        } else if (addr.ss_family == AF_INET6) {
          bound_port_ = ntohs(reinterpret_cast<sockaddr_in6 *>(&addr)->sin6_port);
        }
      }
      freeaddrinfo(res);
      return true;
    }
    CLOSE_SOCKET(fd_);
    fd_ = INVALID_SOCKET_VALUE;
  }
  freeaddrinfo(res);
  return false;
}

void TcpListener::close() {
  if (fd_ != INVALID_SOCKET_VALUE) {
    CLOSE_SOCKET(fd_);
    fd_ = INVALID_SOCKET_VALUE;
  }
}

bool TcpListener::isOpen() const { return fd_ != INVALID_SOCKET_VALUE; }

port_t TcpListener::boundPort() const { return bound_port_; }

std::optional<TcpConnection> TcpListener::acceptConnection(int timeout_ms) {
  if (!isOpen()) {
    return std::nullopt;
  }
  if (timeout_ms >= 0 && !waitReadable(fd_, timeout_ms)) {
    return std::nullopt;
  }
  sockaddr_storage addr{};
  socklen_t len = sizeof(addr);
  socket_t client = ::accept(fd_, reinterpret_cast<::sockaddr *>(&addr), &len);
  if (client == INVALID_SOCKET_VALUE) {
    return std::nullopt;
  }
  char host[NI_MAXHOST];
  char serv[NI_MAXSERV];
  std::string peer;
  if (getnameinfo(reinterpret_cast<::sockaddr *>(&addr), len, host, sizeof(host), serv,
                  sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
    peer = std::string(host) + ":" + serv;
  }
  return TcpConnection(client, peer);
}

HttpResponse httpGet(const std::string &host, port_t port, const std::string &path,
                     int timeout_ms) {
  HttpResponse out;
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *res = nullptr;
  if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0 || !res) {
    out.error = "resolve failed";
    return out;
  }
  socket_t fd = INVALID_SOCKET_VALUE;
  for (addrinfo *p = res; p; p = p->ai_next) {
    fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (fd == INVALID_SOCKET_VALUE) {
      continue;
    }
    if (::connect(fd, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0) {
      break;
    }
    CLOSE_SOCKET(fd);
    fd = INVALID_SOCKET_VALUE;
  }
  freeaddrinfo(res);
  if (fd == INVALID_SOCKET_VALUE) {
    out.error = "connect failed";
    return out;
  }
  TcpConnection conn(fd, host);
  std::ostringstream req;
  req << "GET " << (path.empty() ? "/" : path) << " HTTP/1.1\r\nHost: " << host
      << "\r\nConnection: close\r\n\r\n";
  if (!conn.sendAll(req.str())) {
    out.error = "send failed";
    return out;
  }
  std::string line;
  if (!conn.recvLine(line, timeout_ms) || line.rfind("HTTP/", 0) != 0) {
    out.error = "bad status line";
    return out;
  }
  {
    std::istringstream st(line);
    std::string http;
    st >> http >> out.status;
  }
  while (conn.recvLine(line, timeout_ms) && !line.empty()) {
  }
  std::string body;
  while (conn.recvAvailable(body, timeout_ms, 1024 * 1024)) {
  }
  out.body = std::move(body);
  return out;
}

}  // namespace simple_metricd
