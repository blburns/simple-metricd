/**
 * @file tls.hpp
 * @brief TLS context for HTTPS metrics listener
 */

#pragma once

#include <string>

struct ssl_ctx_st;

namespace simple_metricd {

class TlsContext {
public:
  TlsContext();
  ~TlsContext();

  TlsContext(const TlsContext &) = delete;
  TlsContext &operator=(const TlsContext &) = delete;

  bool loadCertificate(const std::string &cert_file, const std::string &key_file);
  bool loadCa(const std::string &ca_file);
  ssl_ctx_st *serverContext() const { return server_ctx_; }
  ssl_ctx_st *clientContext() const { return client_ctx_; }
  bool enabled() const { return server_ctx_ != nullptr || client_ctx_ != nullptr; }

private:
  ssl_ctx_st *server_ctx_{nullptr};
  ssl_ctx_st *client_ctx_{nullptr};
};

}  // namespace simple_metricd
