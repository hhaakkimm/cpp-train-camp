// =============================================================================
// http_request.hpp — HTTP Request Parser (HEADER)
// =============================================================================
//
// HTTP (HyperText Transfer Protocol) is how web browsers and APIs communicate.
// An HTTP request looks like this:
//
//   GET /kv/mykey HTTP/1.1\r\n       ← Request line (method, path, version)
//   Host: localhost:8080\r\n          ← Header
//   Content-Length: 13\r\n            ← Header (tells us body length)
//   \r\n                              ← Empty line = end of headers
//   Hello, World!                     ← Body (only for PUT/POST requests)
//
// \r\n is a "carriage return + line feed" — the standard line ending in HTTP.
//
// This class parses raw HTTP text into a structured C++ object so we can
// easily access the method, path, headers, and body.
// =============================================================================

#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace mini_redis {

// =============================================================================
// HttpMethod — The type of action the client wants
// =============================================================================
// GET    = "give me data" (read)
// PUT    = "store this data" (create/update)
// DELETE = "remove this data" (delete)
// These map directly to our key-value store operations.
// =============================================================================
enum class HttpMethod {
  GET,
  PUT,
  DELETE,
  UNKNOWN // For methods we don't support
};

// =============================================================================
// HttpRequest — Parsed HTTP request
// =============================================================================
class HttpRequest {
public:
  // ---- Static factory: parse raw HTTP text ----
  // Returns std::optional<HttpRequest>:
  //   - Has value → parsing succeeded
  //   - std::nullopt → the raw text was not valid HTTP
  //
  // ABOUT STRING_VIEW (mentioned for learning):
  // In newer code, you might see std::string_view instead of const string&.
  // string_view is a non-owning reference to a string (just a pointer +
  // length). It's even cheaper than const string& because it doesn't require a
  // std::string object — it can reference a substring, a C-string, etc.
  // We use const string& here for simplicity, but know that string_view
  // is the more modern/efficient choice for read-only string parameters.
  static std::optional<HttpRequest> parse(const std::string &raw_request);

  // ---- Getters ----
  // These methods provide READ-ONLY access to the parsed data.
  // Returning by const reference avoids copying.
  //
  // WHY GETTERS?
  // Instead of making 'method_' public, we provide a getter function.
  // This is ENCAPSULATION — the internal representation can change
  // without breaking code that uses HttpRequest.
  // Example: we could later store method as a string instead of enum,
  // and the getter would convert it — external code wouldn't change.
  HttpMethod method() const;
  const std::string &path() const;
  const std::string &body() const;
  const std::unordered_map<std::string, std::string> &headers() const;

  // Get a specific header value (case-insensitive key lookup)
  // Returns std::nullopt if the header doesn't exist
  std::optional<std::string> get_header(const std::string &name) const;

private:
  // ---- Private constructor ----
  // Only parse() can create HttpRequest objects (factory pattern).
  // This ensures all HttpRequest objects are valid (you can't create
  // a half-parsed or empty request).
  HttpRequest() = default;

  // ---- Helper: convert string to HttpMethod enum ----
  static HttpMethod string_to_method(const std::string &method_str);

  // ---- Parsed fields ----
  HttpMethod method_ = HttpMethod::UNKNOWN;
  std::string path_;
  std::string body_;
  std::unordered_map<std::string, std::string> headers_;
};

} // namespace mini_redis
