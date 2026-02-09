// =============================================================================
// application.hpp — Application Orchestrator (HEADER)
// =============================================================================
//
// This is the TOP-LEVEL class that ties everything together:
//   1. Creates the KeyValueStore
//   2. Creates the ExpiryManager (background cleanup)
//   3. Creates the Router and registers endpoints
//   4. Creates the TcpServer and starts listening
//
// DESIGN PATTERN: COMPOSITION
// The Application class OWNS (composes) all the other components.
// It creates them, configures them, and destroys them in the right order.
//
// WHAT IS COMPOSITION VS INHERITANCE?
// Inheritance: class B inherits from class A ("B IS-A A")
//   - Used when B truly IS a specialized version of A
//   - Example: class Circle : public Shape { ... }
//
// Composition: class B CONTAINS an instance of class A ("B HAS-A A")
//   - Used when B needs A's functionality but isn't a kind of A
//   - Example: class Car { Engine engine_; Wheels wheels_; }
//
// RULE OF THUMB: "Prefer composition over inheritance."
// Composition is more flexible, easier to test, and avoids the
// tight coupling and fragile base class problems of inheritance.
//
// Our Application COMPOSES (has-a) a Store, ExpiryManager, Router, etc.
// It does NOT inherit from any of them.
// =============================================================================

#pragma once

#include "api/kv_handler.hpp"
#include "core/expiry_manager.hpp"
#include "core/key_value_store.hpp"
#include "http/router.hpp"

#include <atomic>
#include <memory> // std::unique_ptr — exclusive-ownership smart pointer

namespace mini_redis {

class Application {
public:
  // Constructor — configures the application
  // port: TCP port to listen on (default 8080)
  // thread_count: number of worker threads (default 4)
  Application(int port = 8080, std::size_t thread_count = 4);

  // Destructor — stops everything
  ~Application();

  // Non-copyable, non-movable
  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&) = delete;
  Application &operator=(Application &&) = delete;

  // Start the application (blocks until stopped)
  void run();

  // Stop the application (can be called from a signal handler)
  void stop();

private:
  // ---- Setup helpers ----
  void setup_routes();
  void handle_connection(Socket client_socket);

  // ---- Configuration ----
  int port_;
  std::size_t thread_count_;

  // ---- Components ----
  // The store and expiry manager are owned directly (not via pointer)
  // because they have a fixed, known lifetime = the Application's lifetime.
  KeyValueStore store_;
  ExpiryManager expiry_manager_;
  Router router_;

  // The KV handler needs the store reference, which is why it's
  // constructed AFTER store_ in the initializer list.
  // IMPORTANT: member variables are initialized in the ORDER THEY ARE
  // DECLARED in the class, NOT the order in the initializer list!
  // So store_ must be declared BEFORE kv_handler_ for the reference to be
  // valid.
  KvHandler kv_handler_;

  // Stop flag for the application
  std::atomic<bool> stop_requested_{false};
};

} // namespace mini_redis
