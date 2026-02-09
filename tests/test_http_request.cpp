// =============================================================================
// test_http_request.cpp — Unit Tests for the HTTP Request Parser
// =============================================================================
//
// These tests verify that our HttpRequest::parse() function correctly
// handles valid and invalid HTTP request strings.
// =============================================================================

#include "http/http_request.hpp"
#include <gtest/gtest.h>

// =============================================================================
// TEST SUITE: HttpRequestTest
// =============================================================================

// --- Test: parse a simple GET request ---
TEST(HttpRequestTest, ParseGetRequest) {
  // A minimal valid HTTP GET request
  // \r\n = HTTP line ending (Carriage Return + Line Feed)
  const std::string raw = "GET /kv/hello HTTP/1.1\r\n"
                          "Host: localhost:8080\r\n"
                          "\r\n";

  const auto request = mini_redis::HttpRequest::parse(raw);

  // Parsing should succeed
  ASSERT_TRUE(request.has_value());

  // Verify parsed fields
  EXPECT_EQ(request->method(), mini_redis::HttpMethod::GET);
  EXPECT_EQ(request->path(), "/kv/hello");
  EXPECT_TRUE(request->body().empty());

  // -> (arrow operator) is shorthand for (*request).method()
  // It dereferences the optional and calls method() on the HttpRequest inside.
}

// --- Test: parse a PUT request with body ---
TEST(HttpRequestTest, ParsePutRequestWithBody) {
  const std::string raw = "PUT /kv/greeting HTTP/1.1\r\n"
                          "Host: localhost:8080\r\n"
                          "Content-Length: 13\r\n"
                          "X-TTL: 60\r\n"
                          "\r\n"
                          "Hello, World!";

  const auto request = mini_redis::HttpRequest::parse(raw);

  ASSERT_TRUE(request.has_value());
  EXPECT_EQ(request->method(), mini_redis::HttpMethod::PUT);
  EXPECT_EQ(request->path(), "/kv/greeting");
  EXPECT_EQ(request->body(), "Hello, World!");

  // Check that headers are parsed correctly
  const auto ttl = request->get_header("X-TTL");
  ASSERT_TRUE(ttl.has_value());
  EXPECT_EQ(ttl.value(), "60");
}

// --- Test: parse a DELETE request ---
TEST(HttpRequestTest, ParseDeleteRequest) {
  const std::string raw = "DELETE /kv/old_key HTTP/1.1\r\n"
                          "Host: localhost\r\n"
                          "\r\n";

  const auto request = mini_redis::HttpRequest::parse(raw);

  ASSERT_TRUE(request.has_value());
  EXPECT_EQ(request->method(), mini_redis::HttpMethod::DELETE);
  EXPECT_EQ(request->path(), "/kv/old_key");
}

// --- Test: empty input returns nullopt ---
TEST(HttpRequestTest, EmptyInputReturnsNullopt) {
  const auto request = mini_redis::HttpRequest::parse("");

  EXPECT_FALSE(request.has_value());
}

// --- Test: malformed request line returns nullopt ---
TEST(HttpRequestTest, MalformedRequestLine) {
  // Only one token instead of three (method path version)
  const auto request = mini_redis::HttpRequest::parse("INVALID\r\n\r\n");

  EXPECT_FALSE(request.has_value());
}

// --- Test: header lookup is case-insensitive ---
TEST(HttpRequestTest, HeaderLookupCaseInsensitive) {
  const std::string raw = "GET /test HTTP/1.1\r\n"
                          "Content-Type: text/plain\r\n"
                          "\r\n";

  const auto request = mini_redis::HttpRequest::parse(raw);

  ASSERT_TRUE(request.has_value());

  // Should find the header regardless of case
  EXPECT_TRUE(request->get_header("content-type").has_value());
  EXPECT_TRUE(request->get_header("Content-Type").has_value());
  EXPECT_TRUE(request->get_header("CONTENT-TYPE").has_value());
}
