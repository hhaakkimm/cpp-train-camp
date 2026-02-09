// =============================================================================
// kv_handler.hpp — REST API Endpoint Handlers (HEADER)
// =============================================================================
//
// This file defines the handler functions for our key-value REST API.
// Each handler corresponds to one endpoint:
//
//   GET    /kv/{key}  → get_key()   — retrieve a value
//   PUT    /kv/{key}  → put_key()   — store a value
//   DELETE /kv/{key}  → delete_key() — remove a value
//   GET    /kv        → list_keys() — list all keys
//
// DESIGN: These functions are "stateless" — they receive the request and
// a reference to the store, do their work, and return a response. They
// don't hold any state themselves. This makes them easy to test and reason
// about.
//
// SEPARATION OF CONCERNS:
// The handler doesn't know about sockets, threads, or HTTP parsing.
// It just receives a parsed HttpRequest and returns an HttpResponse.
// This is the LAYERED ARCHITECTURE pattern:
//
//   Socket Layer → HTTP Layer → Router → Handler → Store
//   (low level)                                  (high level)
//
// Each layer only talks to its immediate neighbors. If we wanted to
// replace TCP with Unix domain sockets, we'd ONLY change the socket layer.
// The handler wouldn't need to change at all.
// =============================================================================

#pragma once

#include "core/key_value_store.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/router.hpp"

namespace mini_redis {

// =============================================================================
// KvHandler — Groups all key-value endpoint handlers
// =============================================================================
// WHY A CLASS INSTEAD OF FREE FUNCTIONS?
// We need each handler to have access to the KeyValueStore. Options:
//   1. Global variable (TERRIBLE — untestable, hidden dependency)
//   2. Pass store as parameter to each handler (verbose)
//   3. Class that holds a reference to the store (clean, testable)
//
// We use option 3. The class constructor takes a reference to the store,
// and each handler method uses it. When registering routes, we bind
// the handler methods to the class instance using lambdas.
// =============================================================================
class KvHandler {
public:
  // Constructor — takes a reference to the store this handler operates on
  explicit KvHandler(KeyValueStore &store);

  // Register all routes with the given router
  void register_routes(Router &router);

  // ---- Endpoint handlers ----
  // Each takes a request + route parameters, returns a response

  // GET /kv/{key} — retrieve a value
  HttpResponse get_key(const HttpRequest &request,
                       const RouteParams &params) const;

  // PUT /kv/{key} — store a value (body = the value, X-TTL header = TTL)
  HttpResponse put_key(const HttpRequest &request, const RouteParams &params);

  // DELETE /kv/{key} — remove a key
  HttpResponse delete_key(const HttpRequest &request,
                          const RouteParams &params);

  // GET /kv — list all keys
  HttpResponse list_keys(const HttpRequest &request,
                         const RouteParams &params) const;

private:
  // Reference to the key-value store (NOT owned by this class)
  KeyValueStore &store_;
};

} // namespace mini_redis
