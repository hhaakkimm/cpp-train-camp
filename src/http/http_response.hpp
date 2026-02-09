// =============================================================================
// http_response.hpp — HTTP Response Builder (HEADER)
// =============================================================================
//
// This class builds HTTP/1.1 response strings to send back to clients.
// It uses the BUILDER PATTERN — a chain of method calls that configure
// the response, then a final build() call to produce the result.
//
// WHAT IS THE BUILDER PATTERN?
// Instead of a constructor with 10 parameters:
//   HttpResponse response(200, "OK", "text/plain", "Hello", ...);
//
// You chain readable method calls:
//   auto response = HttpResponse::ok()
//                       .body("Hello")
//                       .header("Content-Type", "text/plain")
//                       .build();
//
// Benefits:
//   1. Self-documenting (you can see what each value means)
//   2. Order doesn't matter (except build() must be last)
//   3. Optional fields can simply be omitted
// =============================================================================

#pragma once

#include <string>
#include <unordered_map>

namespace mini_redis {

class HttpResponse {
public:
  // ---- Static factories for common status codes ----
  // Each returns a partially-configured HttpResponse that you can
  // further customize with .body() and .header() before calling .build()

  static HttpResponse ok();                 // 200 OK
  static HttpResponse created();            // 201 Created
  static HttpResponse bad_request();        // 400 Bad Request
  static HttpResponse not_found();          // 404 Not Found
  static HttpResponse method_not_allowed(); // 405 Method Not Allowed
  static HttpResponse internal_error();     // 500 Internal Server Error

  // ---- Builder methods ----
  // Each returns a REFERENCE to *this, enabling method chaining:
  //   response.body("hello").header("X-Key", "val").build();
  //
  // WHY RETURN HttpResponse&?
  // By returning a reference to ourselves, the next call in the chain
  // operates on the SAME object. Without this, each call would need
  // a separate statement.
  HttpResponse &body(const std::string &body_content);
  HttpResponse &header(const std::string &name, const std::string &value);

  // ---- Build the final HTTP response string ----
  // This creates the complete HTTP response text ready to send over the wire.
  // Format:
  //   HTTP/1.1 200 OK\r\n
  //   Content-Type: text/plain\r\n
  //   Content-Length: 5\r\n
  //   \r\n
  //   Hello
  std::string build() const;

private:
  // Private constructor — use the static factories instead
  HttpResponse(int status_code, const std::string &status_text);

  int status_code_;         // 200, 404, 500, etc.
  std::string status_text_; // "OK", "Not Found", "Internal Server Error"
  std::string body_;        // Response body content
  std::unordered_map<std::string, std::string> headers_;
};

} // namespace mini_redis
