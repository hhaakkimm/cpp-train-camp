// =============================================================================
// http_request.cpp — HTTP Request Parser (IMPLEMENTATION)
// =============================================================================

#include "http/http_request.hpp"

#include <algorithm> // std::transform — apply a function to each element
#include <sstream>   // std::istringstream — parse strings like a file

// =============================================================================
// Anonymous namespace for internal helper functions
// =============================================================================
// namespace { ... } creates INTERNAL LINKAGE — the function can only be
// used in THIS .cpp file.
//
// WHAT IS AN ANONYMOUS NAMESPACE?
// It's the modern C++ replacement for "static" functions at file scope.
// This prevents name collisions if another .cpp also has a "to_lower" function.
// Everything inside is invisible outside this compilation unit (.cpp file).
// =============================================================================
namespace {

// Convert a string to lowercase (for case-insensitive header matching)
std::string to_lower(std::string str) {
  // std::transform applies a function to each character in the string.
  // str.begin() to str.end() = input range
  // str.begin() = where to write the output (overwriting in place)
  // ::tolower = the C function to convert one character to lowercase
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

} // anonymous namespace

namespace mini_redis {

// =============================================================================
// parse() — Parse raw HTTP text into an HttpRequest object
// =============================================================================
// Example input:
//   "GET /kv/hello HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n"
//
// We parse this in 3 steps:
//   1. Parse the request line (method, path, version)
//   2. Parse headers (key: value pairs, one per line)
//   3. Extract the body (everything after the blank line)
// =============================================================================
std::optional<HttpRequest> HttpRequest::parse(const std::string &raw_request) {
  // Empty request = invalid
  if (raw_request.empty()) {
    return std::nullopt;
  }

  // Create the request object we'll fill in.
  // We can call the private default constructor because parse() is a
  // static MEMBER function of HttpRequest — member functions can access
  // private members of their own class.
  HttpRequest request;

  // Use istringstream to parse the raw text line by line.
  // istringstream makes a string look like an input stream (like cin),
  // so we can use std::getline to read it line by line.
  std::istringstream stream(raw_request);
  std::string line;

  // ---- Step 1: Parse the request line ----
  // getline reads until \n. HTTP uses \r\n, so we get \r at the end.
  if (!std::getline(stream, line)) {
    return std::nullopt; // No request line = invalid
  }

  // Remove trailing \r if present (HTTP line endings are \r\n)
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }

  // Parse "GET /path HTTP/1.1" into three parts using istringstream
  // The >> operator reads whitespace-separated tokens:
  //   "GET /kv/hello HTTP/1.1" → method="GET", path="/kv/hello",
  //   version="HTTP/1.1"
  std::istringstream request_line(line);
  std::string method_str;
  std::string path;
  std::string version;

  if (!(request_line >> method_str >> path >> version)) {
    return std::nullopt; // Couldn't parse all 3 parts
  }

  request.method_ = string_to_method(method_str);
  request.path_ = path;

  // ---- Step 2: Parse headers ----
  // Each header is one line: "Header-Name: value\r\n"
  // An empty line signals the end of headers.
  while (std::getline(stream, line)) {
    // Remove trailing \r
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    // Empty line = end of headers, rest is body
    if (line.empty()) {
      break;
    }

    // Find the colon separator in "Header-Name: value"
    const auto colon_pos = line.find(':');

    // std::string::npos is a special constant meaning "not found"
    if (colon_pos == std::string::npos) {
      continue; // Malformed header line, skip it
    }

    // Extract header name (before colon) — converted to lowercase
    // substr(start, length) extracts a substring
    std::string header_name = to_lower(line.substr(0, colon_pos));

    // Extract header value (after colon), skipping leading whitespace
    std::size_t value_start = colon_pos + 1;
    while (value_start < line.size() && line[value_start] == ' ') {
      ++value_start;
    }
    std::string header_value = line.substr(value_start);

    // Store in the map (name is lowercase for case-insensitive lookup)
    request.headers_[header_name] = header_value;
  }

  // ---- Step 3: Extract body (everything remaining in the stream) ----
  // std::istreambuf_iterator reads raw characters from the stream buffer.
  // Constructing a string from two iterators reads ALL remaining content.
  // The extra parentheses around the first iterator avoid the "most vexing
  // parse" — a C++ gotcha where the compiler thinks you're declaring a
  // function.
  std::string remaining((std::istreambuf_iterator<char>(stream)),
                        std::istreambuf_iterator<char>());
  request.body_ = remaining;

  return request;
}

// =============================================================================
// Getter implementations — simple one-liners
// =============================================================================
// These just return the private member variables.
// "const" at the end guarantees we don't modify the object.
// =============================================================================
HttpMethod HttpRequest::method() const { return method_; }

const std::string &HttpRequest::path() const { return path_; }

const std::string &HttpRequest::body() const { return body_; }

const std::unordered_map<std::string, std::string> &
HttpRequest::headers() const {
  return headers_;
}

// =============================================================================
// get_header() — Case-insensitive header lookup
// =============================================================================
std::optional<std::string>
HttpRequest::get_header(const std::string &name) const {
  // Convert the requested name to lowercase for case-insensitive matching
  // (all header names were stored in lowercase during parsing)
  const auto it = headers_.find(to_lower(name));

  if (it == headers_.end()) {
    return std::nullopt;
  }

  // it->second is the VALUE in the key-value pair (it->first is the key)
  return it->second;
}

// =============================================================================
// string_to_method() — Convert HTTP method string to enum
// =============================================================================
HttpMethod HttpRequest::string_to_method(const std::string &method_str) {
  if (method_str == "GET")
    return HttpMethod::GET;
  if (method_str == "PUT")
    return HttpMethod::PUT;
  if (method_str == "DELETE")
    return HttpMethod::DELETE;
  return HttpMethod::UNKNOWN;
}

} // namespace mini_redis
