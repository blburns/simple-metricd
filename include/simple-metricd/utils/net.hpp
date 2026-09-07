/**
 * @file net.hpp
 * @brief TCP and minimal HTTP helpers
 */

#pragma once

#include "simple-metricd/utils/platform.hpp"
#include <cstdint>
#include <optional>
#include <string>

namespace simple_metricd {

class TlsContext;

struct HttpResponse {
  int status{0};
  std::string body;
  std::string error;
};

bool initializeSockets();
void shutdownSockets();

bool parseHostPort(const std::string &target, std::string &host, port_t &port,
                   port_t default_port);

class TcpConnection {
public:
  TcpConnection() = default;
  explicit TcpConnection(socket_t fd, std::string peer = {});
  TcpConnection(TcpConnection &&other) noexcept;
  TcpConnection &operator=(TcpConnection &&other) noexcept;
  ~TcpConnection();

  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

  bool valid() const;
  void close();
  socket_t native() const { return fd_; }
  const std::string &peer() const { return peer_; }
  bool tls() const { return ssl_ != nullptr; }

  bool handshakeTls(const TlsContext &ctx, bool server);
  bool sendAll(const std::string &data);
  bool recvLine(std::string &line, int timeout_ms);
  bool recvAvailable(std::string &data, int timeout_ms, std::size_t max_bytes);

private:
  socket_t fd_{INVALID_SOCKET_VALUE};
  std::string peer_;
  void *ssl_{nullptr};
};

class TcpListener {
public:
  TcpListener() = default;
  ~TcpListener();

  TcpListener(const TcpListener &) = delete;
  TcpListener &operator=(const TcpListener &) = delete;

  bool bindAndListen(const std::string &address, port_t port);
  void close();
  bool isOpen() const;
  port_t boundPort() const;
  std::optional<TcpConnection> acceptConnection(int timeout_ms = -1);

private:
  socket_t fd_{INVALID_SOCKET_VALUE};
  port_t bound_port_{0};
};

HttpResponse httpGet(const std::string &host, port_t port, const std::string &path,
                     int timeout_ms);

}  // namespace simple_metricd
