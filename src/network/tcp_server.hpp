// =============================================================================
// tcp_server.hpp — TCP Server (HEADER)
// =============================================================================
//
// This class creates a listening TCP socket, accepts incoming client
// connections, and dispatches each one to a handler function via
// the thread pool.
//
// ARCHITECTURE:
//   1. main thread: creates Socket → binds to port → listens
//   2. accept loop: waits for connections, pushes them to the thread pool
//   3. worker threads: each handles one client request (read → process →
//   respond)
//
// This is called the "thread-per-request" model. It's simple and works
// well for moderate loads (hundreds of concurrent connections).
// For millions of connections, you'd use async I/O (epoll/kqueue), but
// that's an advanced topic beyond this project's scope.
// =============================================================================

#pragma once

#include "network/socket.hpp"
#include "util/thread_pool.hpp"

#include <atomic>     // std::atomic
#include <functional> // std::function

namespace mini_redis {

// Type alias for the function that handles a client connection.
// It receives a Socket and does whatever is needed (read request, process,
// respond).
//
// WHAT IS A TYPE ALIAS?
// "using X = Y;" gives type Y a new name X. This makes code more readable:
//   void start(std::function<void(Socket)> handler);   ← verbose
//   void start(ConnectionHandler handler);              ← clear intent
using ConnectionHandler = std::function<void(Socket)>;

class TcpServer {
public:
  // Constructor takes the port number and thread count
  TcpServer(int port, std::size_t thread_count);

  // Destructor — stops the server
  ~TcpServer();

  // Non-copyable, non-movable
  TcpServer(const TcpServer &) = delete;
  TcpServer &operator=(const TcpServer &) = delete;
  TcpServer(TcpServer &&) = delete;
  TcpServer &operator=(TcpServer &&) = delete;

  // Start accepting connections.
  // handler = the function that processes each client connection.
  // This function BLOCKS — it runs the accept loop until stop() is called.
  void start(ConnectionHandler handler);

  // Stop the server (signal the accept loop to exit)
  void stop();

private:
  // Port to listen on (e.g., 8080)
  int port_;

  // The thread pool for handling connections concurrently
  ThreadPool thread_pool_;

  // Flag to signal the accept loop to stop
  std::atomic<bool> stop_requested_{false};
};

} // namespace mini_redis
