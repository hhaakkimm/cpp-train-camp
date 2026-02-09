// =============================================================================
// tcp_server.cpp — TCP Server (IMPLEMENTATION)
// =============================================================================

#include "network/tcp_server.hpp"
#include "util/logger.hpp"

#include <string>

namespace mini_redis {

// =============================================================================
// Constructor
// =============================================================================
TcpServer::TcpServer(int port, std::size_t thread_count)
    : port_(port), thread_pool_(thread_count) {
  // All initialization done in the member initializer list.
  // port_ is set, and thread_pool_ is constructed with 'thread_count' workers.
}

// =============================================================================
// Destructor — ensure the server is stopped
// =============================================================================
TcpServer::~TcpServer() { stop(); }

// =============================================================================
// start() — Create socket, bind, listen, and accept connections
// =============================================================================
// WHAT IS std::shared_ptr<T>?
// In competitive programming, you never think about who "owns" data or
// when to free memory. In industrial C++, ownership is EVERYTHING.
//
// std::shared_ptr is a SMART POINTER that:
//   - Wraps a raw pointer to an object
//   - Counts how many shared_ptrs point to the object (reference count)
//   - When the LAST shared_ptr is destroyed, the object is automatically
//   deleted
//
// WHY do we use shared_ptr for the handler here?
// The handler needs to be shared between the TcpServer (which stores it)
// and each task lambda submitted to the thread pool. We can't use a
// regular reference because the handler might go out of scope while
// tasks are still running. shared_ptr keeps the handler alive as long
// as any task still needs it.
//
// There are 3 types of smart pointers:
//   std::unique_ptr — exclusive ownership (ONE owner, most common, zero
//   overhead) std::shared_ptr — shared ownership (MULTIPLE owners, reference
//   counted) std::weak_ptr   — observing a shared_ptr without keeping it alive
// =============================================================================
void TcpServer::start(ConnectionHandler handler) {
  // Create a TCP socket using our RAII wrapper
  auto server_socket = Socket::create_tcp();

  // "auto" lets the compiler INFER the type. Here:
  // auto = std::optional<Socket>
  // Using auto is idiomatic in modern C++ for long type names.
  if (!server_socket.has_value()) {
    Logger::error("Failed to create server socket");
    return;
  }

  // Bind the socket to our port
  if (!server_socket->bind_to(port_)) {
    Logger::error("Failed to bind to port " + std::to_string(port_));
    return;
  }

  // Start listening for incoming connections
  if (!server_socket->start_listening()) {
    Logger::error("Failed to start listening");
    return;
  }

  Logger::info("Mini Redis server listening on port " + std::to_string(port_));

  // Wrap handler in shared_ptr so lambdas can share it safely
  auto shared_handler = std::make_shared<ConnectionHandler>(std::move(handler));

  // ---- The Accept Loop ----
  // This loop runs forever (until stop() is called), accepting new
  // connections and dispatching them to worker threads.
  while (!stop_requested_.load()) {
    // accept_connection() BLOCKS until a client connects
    auto client_socket = server_socket->accept_connection();

    // If accept failed or we're stopping, skip this iteration
    if (!client_socket.has_value()) {
      continue;
    }

    // Submit a task to the thread pool to handle this connection.
    // We use std::make_shared to create a shared_ptr to the client
    // Socket, because the lambda might outlive this loop iteration.
    //
    // WHY CAN'T WE JUST CAPTURE client_socket BY REFERENCE?
    // The lambda runs in another thread. By the time the worker thread
    // picks it up, client_socket might have been destroyed or overwritten
    // by the next iteration. Shared pointers keep the socket alive.
    auto client = std::make_shared<Socket>(std::move(client_socket.value()));

    thread_pool_.submit([shared_handler, client]() {
      // Move the socket out of shared_ptr for the handler
      // This is safe because each task gets its own shared_ptr copy
      (*shared_handler)(std::move(*client));
    });
  }

  Logger::info("Server accept loop stopped");
}

// =============================================================================
// stop() — Signal the accept loop to exit
// =============================================================================
void TcpServer::stop() {
  stop_requested_.store(true);

  // NOTE: The accept loop might be blocked in accept_connection().
  // We rely on the destructor of the server_socket (when start() returns)
  // to close the listening socket, which will cause accept() to return
  // with an error, breaking the loop.
  Logger::info("Server stop requested");
}

} // namespace mini_redis
