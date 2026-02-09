// =============================================================================
// router.hpp — HTTP Request Router (HEADER)
// =============================================================================
//
// WHAT IS A ROUTER?
// A router maps incoming HTTP requests to the correct handler function
// based on the request method (GET/PUT/DELETE) and path (/kv/mykey).
//
// Think of it like a switchboard operator:
//   "GET /kv/hello"  → calls the "get key-value" handler
//   "PUT /kv/hello"  → calls the "set key-value" handler
//   "GET /kv"        → calls the "list all keys" handler
//   "GET /unknown"   → returns 404 Not Found
//
// PATH PARAMETERS:
// Our router supports extracting values from the URL path.
// For example, the route "/kv/" with path "/kv/hello" extracts "hello"
// as the key parameter. This is how REST APIs work — the resource
// identifier is part of the URL.
// =============================================================================

#pragma once

#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <functional>
#include <string>
#include <vector>

namespace mini_redis {

// =============================================================================
// RouteParams — Extra data extracted from the URL
// =============================================================================
// For now, this just holds the "path suffix" — the part of the URL
// after the route prefix. For "/kv/{key}", this would be "hello"
// when the URL is "/kv/hello".
// =============================================================================
struct RouteParams {
  std::string path_suffix; // The dynamic part of the URL
};

// =============================================================================
// Handler function type — takes a request + params, returns a response
// =============================================================================
// WHY std::function instead of a function pointer?
// Function pointers (void (*)(int)) can only hold plain functions.
// std::function can hold:
//   - Regular functions
//   - Lambdas (anonymous functions)
//   - Lambdas with captures (closures)
//   - Functors (objects with operator())
//   - Member functions (via std::bind or lambda)
//
// Since our handlers need to CAPTURE the KeyValueStore reference,
// we need lambdas with captures, which requires std::function.
// =============================================================================
using HandlerFunc =
    std::function<HttpResponse(const HttpRequest &, const RouteParams &)>;

class Router {
public:
  // ---- Register a route ----
  // method:    which HTTP method to match (GET, PUT, DELETE)
  // prefix:    the URL prefix to match (e.g., "/kv/")
  // handler:   the function to call when matched
  void add_route(HttpMethod method, const std::string &prefix,
                 HandlerFunc handler);

  // ---- Route a request ----
  // Finds the matching handler for the given request and calls it.
  // If no route matches, returns 404 Not Found.
  HttpResponse route(const HttpRequest &request) const;

private:
  // ---- Route entry ----
  // Each registered route is stored as a struct with the method,
  // prefix pattern, and handler function.
  struct Route {
    HttpMethod method;
    std::string prefix;
    HandlerFunc handler;
  };

  // All registered routes
  std::vector<Route> routes_;
};

} // namespace mini_redis
