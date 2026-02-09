// =============================================================================
// application.cpp — Application Orchestrator (IMPLEMENTATION)
// =============================================================================

#include "app/application.hpp"
#include "network/tcp_server.hpp"
#include "util/logger.hpp"

namespace mini_redis {

// =============================================================================
// Constructor — Initialize all components
// =============================================================================
// MEMBER INITIALIZATION ORDER:
// Members are initialized in the order they appear in the CLASS DEFINITION,
// not in the order written in the initializer list. This matters because
// kv_handler_ takes a reference to store_, so store_ MUST be initialized first.
//
// Common bug: if you declare kv_handler_ BEFORE store_ in the class,
// the reference would be to an uninitialized store_ → crash!
// Modern compilers warn about this with -Wall.
// =============================================================================
Application::Application(int port, std::size_t thread_count)
    : port_(port), thread_count_(thread_count), store_(),
      expiry_manager_(store_) // Pass store_ by reference
      ,
      router_(), kv_handler_(store_) { // Pass store_ by reference
  // Setup routes in constructor body (after all members are initialized)
  setup_routes();
}

// =============================================================================
// Destructor — Stop everything in the right order
// =============================================================================
// Members are destroyed in REVERSE order of declaration.
// So kv_handler_ is destroyed first, then router_, then expiry_manager_,
// then store_. This is correct: the expiry manager is stopped before
// the store it references is destroyed.
// =============================================================================
Application::~Application() { stop(); }

// =============================================================================
// setup_routes() — Register all API endpoints
// =============================================================================
void Application::setup_routes() {
  kv_handler_.register_routes(router_);
  Logger::info("All routes configured");
}

// =============================================================================
// run() — Start the application
// =============================================================================
void Application::run() {
  Logger::info("=== Mini Redis v1.0 ===");

  // Start the background expiry manager
  expiry_manager_.start();

  // Create and start the TCP server
  // This will BLOCK in the accept loop until stop() is called
  TcpServer server(port_, thread_count_);

  // Pass our connection handler as a lambda.
  // [this] captures the Application pointer so the lambda can call
  // handle_connection() on this Application instance.
  server.start([this](Socket client_socket) {
    handle_connection(std::move(client_socket));
  });
}

// =============================================================================
// stop() — Gracefully shut down the application
// =============================================================================
void Application::stop() {
  if (stop_requested_.exchange(true)) {
    // exchange() atomically sets the flag to true and returns the OLD value.
    // If the old value was already true, stop() was already called — do
    // nothing.
    return;
  }

  Logger::info("Shutting down gracefully...");
  expiry_manager_.stop();
}

// =============================================================================
// handle_connection() — Process one client HTTP request
// =============================================================================
// This function is called by a worker thread for each incoming connection.
// Flow:
//   1. Read the raw HTTP data from the socket
//   2. Parse it into an HttpRequest
//   3. Route it to the correct handler
//   4. Send the HttpResponse back
// =============================================================================
void Application::handle_connection(Socket client_socket) {
  // Step 1: Read raw data from the client
  const std::string raw_request = client_socket.read_all();

  if (raw_request.empty()) {
    // Client disconnected or error — nothing to do
    return;
  }

  // Step 2: Parse the raw HTTP text into a structured request
  const auto request = HttpRequest::parse(raw_request);

  if (!request.has_value()) {
    // Couldn't parse the request — send 400 Bad Request
    const std::string response =
        HttpResponse::bad_request().body("Invalid HTTP request").build();
    client_socket.write_all(response);
    return;
  }

  // Step 3: Route the request to the correct handler
  const HttpResponse response = router_.route(request.value());

  // Step 4: Send the response back to the client
  client_socket.write_all(response.build());

  // When this function returns, 'client_socket' goes out of scope
  // and its destructor closes the connection. RAII at work!
}

} // namespace mini_redis
