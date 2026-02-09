// =============================================================================
// kv_handler.cpp — REST API Endpoint Handlers (IMPLEMENTATION)
// =============================================================================

#include "api/kv_handler.hpp"
#include "util/logger.hpp"

#include <sstream> // for building the key list response

namespace mini_redis {

// =============================================================================
// Constructor
// =============================================================================
KvHandler::KvHandler(KeyValueStore &store) : store_(store) {
  // Just stores the reference to the store.
  // The store itself is owned by the Application class.
}

// =============================================================================
// register_routes() — Wire up URL patterns to handler methods
// =============================================================================
// We use LAMBDAS to bridge the router's HandlerFunc signature with our
// member functions. Each lambda captures 'this' so it can call our methods.
//
// WHY LAMBDAS HERE?
// The router expects: std::function<HttpResponse(const HttpRequest&, const
// RouteParams&)> Our methods are: HttpResponse KvHandler::get_key(const
// HttpRequest&, const RouteParams&)
//
// Member functions have an implicit 'this' parameter, so we can't pass them
// directly. The lambda captures 'this' and forwards the call.
// =============================================================================
void KvHandler::register_routes(Router &router) {
  // GET /kv/ → get_key (note: the router will extract the key from the suffix)
  router.add_route(HttpMethod::GET, "/kv/",
                   [this](const HttpRequest &req, const RouteParams &params) {
                     return get_key(req, params);
                   });

  // PUT /kv/ → put_key
  router.add_route(HttpMethod::PUT, "/kv/",
                   [this](const HttpRequest &req, const RouteParams &params) {
                     return put_key(req, params);
                   });

  // DELETE /kv/ → delete_key
  router.add_route(HttpMethod::DELETE, "/kv/",
                   [this](const HttpRequest &req, const RouteParams &params) {
                     return delete_key(req, params);
                   });

  // GET /kv → list_keys (exact match, no trailing slash)
  // IMPORTANT: This must be registered AFTER the /kv/ routes above!
  // Because /kv/ is more specific than /kv, and first-match wins.
  // Actually, /kv won't match "/kv/" prefix (it doesn't start with "/kv/").
  // So order doesn't matter here, but it's good practice to register
  // more specific routes first.
  router.add_route(HttpMethod::GET, "/kv",
                   [this](const HttpRequest &req, const RouteParams &params) {
                     return list_keys(req, params);
                   });

  Logger::info("KV handler routes registered");
}

// =============================================================================
// GET /kv/{key} — Retrieve a value by key
// =============================================================================
HttpResponse KvHandler::get_key(const HttpRequest & /*request*/,
                                const RouteParams &params) const {
  // The key is the path suffix extracted by the router.
  // Example: URL "/kv/hello" with prefix "/kv/" → suffix = "hello"
  const std::string &key = params.path_suffix;

  // Validate the key
  if (key.empty()) {
    return HttpResponse::bad_request().body("Key cannot be empty");
  }

  // Look up the value in the store
  const auto value = store_.get(key);

  // If the key exists, return it. If not, return 404.
  if (value.has_value()) {
    return HttpResponse::ok().body(value.value());
  }

  return HttpResponse::not_found().body("Key not found: " + key);
}

// =============================================================================
// PUT /kv/{key} — Store a value
// =============================================================================
// The value comes from the HTTP request body.
// An optional X-TTL header specifies the TTL in seconds.
// =============================================================================
HttpResponse KvHandler::put_key(const HttpRequest &request,
                                const RouteParams &params) {
  const std::string &key = params.path_suffix;

  if (key.empty()) {
    return HttpResponse::bad_request().body("Key cannot be empty");
  }

  // The request body IS the value to store
  const std::string &value = request.body();

  // Check for optional X-TTL header (Time-To-Live in seconds)
  int ttl_seconds = 0;
  const auto ttl_header = request.get_header("X-TTL");

  if (ttl_header.has_value()) {
    // Convert the header string to an integer
    // std::stoi = "string to integer" — throws if the string is not a number
    try {
      ttl_seconds = std::stoi(ttl_header.value());
    } catch (const std::exception & /*e*/) {
      // If X-TTL is not a valid number, ignore it (use default 0)
      Logger::warning("Invalid X-TTL header value: " + ttl_header.value());
    }
  }

  // Store the key-value pair
  store_.set(key, value, ttl_seconds);

  return HttpResponse::created().body("OK");
}

// =============================================================================
// DELETE /kv/{key} — Remove a key
// =============================================================================
HttpResponse KvHandler::delete_key(const HttpRequest & /*request*/,
                                   const RouteParams &params) {
  const std::string &key = params.path_suffix;

  if (key.empty()) {
    return HttpResponse::bad_request().body("Key cannot be empty");
  }

  const bool removed = store_.remove(key);

  if (removed) {
    return HttpResponse::ok().body("Deleted: " + key);
  }

  return HttpResponse::not_found().body("Key not found: " + key);
}

// =============================================================================
// GET /kv — List all keys
// =============================================================================
HttpResponse KvHandler::list_keys(const HttpRequest & /*request*/,
                                  const RouteParams & /*params*/) const {
  const auto all_keys = store_.keys();

  // Build a newline-separated list of keys
  std::ostringstream response_body;

  for (std::size_t i = 0; i < all_keys.size(); ++i) {
    response_body << all_keys[i];

    // Add newline between keys (but not after the last one)
    if (i + 1 < all_keys.size()) {
      response_body << "\n";
    }
  }

  return HttpResponse::ok().body(response_body.str());
}

} // namespace mini_redis
