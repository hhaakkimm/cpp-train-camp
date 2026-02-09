// =============================================================================
// http_response.cpp — HTTP Response Builder (IMPLEMENTATION)
// =============================================================================

#include "http/http_response.hpp"

#include <sstream> // std::ostringstream for building the response string

namespace mini_redis {

// =============================================================================
// Private constructor
// =============================================================================
HttpResponse::HttpResponse(int status_code, const std::string &status_text)
    : status_code_(status_code), status_text_(status_text) {
  // Set a default Content-Type header.
  // "text/plain" means the body is plain text (not HTML, not JSON).
  // Clients (like curl or web browsers) use this to know how to
  // display the response.
  headers_["Content-Type"] = "text/plain";
}

// =============================================================================
// Static factories — create responses with common status codes
// =============================================================================
// These are SHORT, READABLE, and SELF-DOCUMENTING.
// Compare: HttpResponse(200, "OK") vs HttpResponse::ok()
// The factory name tells you EXACTLY what kind of response it is.
// =============================================================================
HttpResponse HttpResponse::ok() { return HttpResponse(200, "OK"); }

HttpResponse HttpResponse::created() { return HttpResponse(201, "Created"); }

HttpResponse HttpResponse::bad_request() {
  return HttpResponse(400, "Bad Request");
}

HttpResponse HttpResponse::not_found() {
  return HttpResponse(404, "Not Found");
}

HttpResponse HttpResponse::method_not_allowed() {
  return HttpResponse(405, "Method Not Allowed");
}

HttpResponse HttpResponse::internal_error() {
  return HttpResponse(500, "Internal Server Error");
}

// =============================================================================
// body() — Set the response body (builder method)
// =============================================================================
// Returns *this so you can chain: response.body("hello").header(...)
// =============================================================================
HttpResponse &HttpResponse::body(const std::string &body_content) {
  body_ = body_content;
  return *this; // Return reference to this object for chaining
}

// =============================================================================
// header() — Add a custom response header (builder method)
// =============================================================================
HttpResponse &HttpResponse::header(const std::string &name,
                                   const std::string &value) {
  headers_[name] = value;
  return *this;
}

// =============================================================================
// build() — Serialize the response to HTTP/1.1 text format
// =============================================================================
// Produces a string like:
//   HTTP/1.1 200 OK\r\n
//   Content-Type: text/plain\r\n
//   Content-Length: 12\r\n
//   \r\n
//   Hello World!
// =============================================================================
std::string HttpResponse::build() const {
  std::ostringstream response;

  // ---- Status line ----
  // "HTTP/1.1" = protocol version (we support HTTP 1.1)
  response << "HTTP/1.1 " << status_code_ << " " << status_text_ << "\r\n";

  // ---- Content-Length header ----
  // This tells the client how many bytes the body is.
  // Without it, the client doesn't know when the body ends!
  // We always include this, even for empty bodies (Content-Length: 0).
  response << "Content-Length: " << body_.size() << "\r\n";

  // ---- Connection: close header ----
  // This tells the client to close the connection after this response.
  // In HTTP/1.1, connections are "keep-alive" by default (reused for
  // multiple requests). But our simple server handles one request per
  // connection, so we explicitly close it.
  response << "Connection: close\r\n";

  // ---- Custom headers ----
  // Range-based for loop over the unordered_map.
  // auto& [name, value] uses structured bindings (C++17) to unpack
  // each key-value pair.
  for (const auto &[name, value] : headers_) {
    response << name << ": " << value << "\r\n";
  }

  // ---- Empty line separating headers from body ----
  response << "\r\n";

  // ---- Body ----
  response << body_;

  return response.str();
}

} // namespace mini_redis
