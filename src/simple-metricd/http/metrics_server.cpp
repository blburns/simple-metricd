/**
 * @file metrics_server.cpp
 */

#include "simple-metricd/http/metrics_server.hpp"

#include "simple-metricd/http/exposition.hpp"
#include "simple-metricd/http/ingest.hpp"
#include "simple-metricd/version.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace simple_metricd {

namespace {

std::string lowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

bool isIngestPath(const std::string &path) {
  return path == "/ingest" || path == "/ingest/" || path == "/api/v1/import" ||
         path == "/api/v1/import/";
}

}  // namespace

MetricsServer::MetricsServer(std::string listen_address, port_t listen_port,
                             MetricRegistry &registry)
    : listen_address_(std::move(listen_address)), listen_port_(listen_port),
      registry_(registry) {}

MetricsServer::~MetricsServer() { stop(); }

bool MetricsServer::start() {
  if (!listener_.bindAndListen(listen_address_, listen_port_)) {
    return false;
  }
  bound_port_ = listener_.boundPort() ? listener_.boundPort() : listen_port_;
  running_ = true;
  thread_ = std::thread([this]() { acceptLoop(); });
  return true;
}

void MetricsServer::stop() {
  running_ = false;
  listener_.close();
  if (thread_.joinable()) {
    thread_.join();
  }
}

void MetricsServer::acceptLoop() {
  while (running_) {
    auto conn = listener_.acceptConnection(500);
    if (!conn) {
      continue;
    }
    if (tls_ && tls_->enabled()) {
      if (!conn->handshakeTls(*tls_, true)) {
        continue;
      }
    }
    handleClient(std::move(*conn));
  }
}

void MetricsServer::handleClient(TcpConnection connection) {
  std::string line;
  if (!connection.recvLine(line, 5000)) {
    return;
  }
  std::istringstream req(line);
  std::string method;
  std::string path;
  std::string http;
  req >> method >> path >> http;

  std::size_t content_length = 0;
  std::string authorization;
  while (connection.recvLine(line, 2000) && !line.empty()) {
    const auto colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    const std::string key = lowerCopy(line.substr(0, colon));
    if (key == "content-length") {
      try {
        content_length = static_cast<std::size_t>(std::stoul(line.substr(colon + 1)));
      } catch (...) {
        content_length = 0;
      }
    } else if (key == "authorization") {
      authorization = line.substr(colon + 1);
      while (!authorization.empty() &&
             std::isspace(static_cast<unsigned char>(authorization.front()))) {
        authorization.erase(authorization.begin());
      }
    }
  }

  std::string request_body;
  if (content_length > 0) {
    if (content_length > 1024 * 1024 ||
        !connection.recvExact(request_body, content_length, 5000)) {
      return;
    }
  }

  const std::string path_only = path.substr(0, path.find('?'));
  if (!acl_.allow(connection.peer(), path_only, authorization)) {
    const std::string forbidden = "forbidden\n";
    std::ostringstream resp;
    resp << "HTTP/1.1 403 Forbidden\r\nContent-Length: " << forbidden.size()
         << "\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"
         << forbidden;
    connection.sendAll(resp.str());
    return;
  }
  if (!rate_limiter_.allow(connection.peer())) {
    const std::string body = "rate limit exceeded\n";
    std::ostringstream resp;
    resp << "HTTP/1.1 429 Too Many Requests\r\nContent-Length: " << body.size()
         << "\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"
         << body;
    connection.sendAll(resp.str());
    return;
  }

  std::string body;
  std::string content_type = "text/plain; version=0.0.4";
  int code = 200;

  if (method == "POST" && isIngestPath(path_only)) {
    content_type = "text/plain";
    const IngestResult result = ingestBody(registry_, request_body);
    if (result.rejected > 0 && result.accepted == 0) {
      code = 400;
      body = "ingest failed: " + result.last_error + "\n";
    } else {
      std::ostringstream ok;
      ok << "accepted=" << result.accepted << " rejected=" << result.rejected << "\n";
      body = ok.str();
    }
  } else if (method != "GET") {
    body = "method not allowed\n";
    code = 405;
  } else if (path_only == "/metrics" || path_only == "/metrics/") {
    body = renderPrometheusText(registry_, true);
  } else if (path_only == "/healthz" || path_only == "/healthz/") {
    body = "ok\n";
  } else if (path_only == "/status" || path_only == "/status/") {
    content_type = "application/json";
    std::ostringstream json;
    json << "{\"version\":\"" << kVersion << "\",\"metrics\":" << registry_.size()
         << ",\"port\":" << bound_port_ << "}";
    body = json.str();
  } else {
    body = "not found\n";
    code = 404;
  }

  std::ostringstream resp;
  resp << "HTTP/1.1 " << code;
  if (code == 200) {
    resp << " OK";
  } else if (code == 400) {
    resp << " Bad Request";
  } else if (code == 403) {
    resp << " Forbidden";
  } else if (code == 405) {
    resp << " Method Not Allowed";
  } else {
    resp << " Not Found";
  }
  resp << "\r\nContent-Length: " << body.size() << "\r\nContent-Type: " << content_type
       << "\r\nConnection: close\r\n\r\n"
       << body;
  connection.sendAll(resp.str());
}

}  // namespace simple_metricd
