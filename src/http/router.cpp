// =============================================================================
// router.cpp — HTTP Request Router (IMPLEMENTATION)
// =============================================================================

#include "http/router.hpp"
#include "util/logger.hpp"

namespace mini_redis {

// =============================================================================
// add_route() — Register a new route
// =============================================================================
// Adds a route entry to our routes_ vector. When a request comes in,
// we'll check each route to find a match.
// =============================================================================
void Router::add_route(HttpMethod method, const std::string &prefix,
                       HandlerFunc handler) {
  // Push a Route struct into the vector.
  // We use the aggregate initialization syntax { ... } to create
  // the struct inline. std::move(handler) transfers ownership of
  // the handler function without copying it.
  routes_.push_back(Route{method, prefix, std::move(handler)});

  Logger::info("Route registered: " + prefix);
}

// =============================================================================
// route() — Find and execute the matching handler
// =============================================================================
// Algorithm:
//   1. For each registered route, check if the method AND path prefix match
//   2. If a match is found, extract the path suffix and call the handler
//   3. If no match is found, return 404
//
// NOTE: routes are checked in ORDER — first match wins. This means more
// specific routes should be registered before more general ones.
// For example, register "/kv/" before "/" to avoid "/" matching everything.
// =============================================================================
HttpResponse Router::route(const HttpRequest &request) const {
  // Get the request path (e.g., "/kv/hello")
  const std::string &path = request.path();

  // Check each route for a match
  for (const auto &route : routes_) {
    // Check 1: Does the HTTP method match?
    if (route.method != request.method()) {
      continue; // Method doesn't match, try next route
    }

    // Check 2: Does the request path START WITH the route prefix?
    // std::string::find() returns the position of the substring.
    // If it returns 0, the path STARTS with the prefix.
    //
    // Example:
    //   path = "/kv/hello", prefix = "/kv/"
    //   path.find("/kv/") returns 0 → match!
    //
    //   path = "/status", prefix = "/kv/"
    //   path.find("/kv/") returns std::string::npos → no match
    if (path.find(route.prefix) != 0) {
      continue; // Path doesn't match, try next route
    }

    // Match found! Extract the path suffix (the part after the prefix)
    RouteParams params;
    params.path_suffix = path.substr(route.prefix.size());
    // Example: path="/kv/hello", prefix="/kv/" → suffix="hello"

    // Call the handler function and return its response
    return route.handler(request, params);
  }

  // No route matched — return 404 Not Found
  Logger::warning("No route matched for: " + path);
  return HttpResponse::not_found().body("Not Found: " + path);
}

} // namespace mini_redis
