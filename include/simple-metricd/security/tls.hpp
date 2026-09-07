/**
 * @file tls.hpp
 * @brief TLS context placeholder (enabled in Milestone 5)
 */

#pragma once

#include <string>

namespace simple_metricd {

class TlsContext {
public:
  bool load(const std::string &cert_file, const std::string &key_file,
            const std::string &ca_file = {});
  bool enabled() const { return enabled_; }
  void *serverContext() const { return server_ctx_; }
  void *clientContext() const { return client_ctx_; }

private:
  bool enabled_{false};
  void *server_ctx_{nullptr};
  void *client_ctx_{nullptr};
};

}  // namespace simple_metricd
