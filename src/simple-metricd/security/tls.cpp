/**
 * @file tls.cpp
 */

#include "simple-metricd/security/tls.hpp"

#ifdef SIMPLE_METRICD_SSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#endif

namespace simple_metricd {

TlsContext::TlsContext() {
#ifdef SIMPLE_METRICD_SSL
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();
  client_ctx_ = SSL_CTX_new(TLS_client_method());
  server_ctx_ = SSL_CTX_new(TLS_server_method());
#endif
}

TlsContext::~TlsContext() {
#ifdef SIMPLE_METRICD_SSL
  if (server_ctx_) {
    SSL_CTX_free(server_ctx_);
  }
  if (client_ctx_) {
    SSL_CTX_free(client_ctx_);
  }
#endif
}

bool TlsContext::loadCertificate(const std::string &cert_file, const std::string &key_file) {
#ifdef SIMPLE_METRICD_SSL
  if (!server_ctx_) {
    return false;
  }
  if (SSL_CTX_use_certificate_file(server_ctx_, cert_file.c_str(), SSL_FILETYPE_PEM) != 1) {
    return false;
  }
  return SSL_CTX_use_PrivateKey_file(server_ctx_, key_file.c_str(), SSL_FILETYPE_PEM) == 1;
#else
  (void)cert_file;
  (void)key_file;
  return false;
#endif
}

bool TlsContext::loadCa(const std::string &ca_file) {
#ifdef SIMPLE_METRICD_SSL
  if (!client_ctx_) {
    return false;
  }
  return SSL_CTX_load_verify_locations(client_ctx_, ca_file.c_str(), nullptr) == 1;
#else
  (void)ca_file;
  return false;
#endif
}

}  // namespace simple_metricd
