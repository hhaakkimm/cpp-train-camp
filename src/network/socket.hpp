// =============================================================================
// socket.hpp — RAII Socket Wrapper (HEADER)
// =============================================================================
//
// WHAT IS A SOCKET?
// A socket is the OS-level endpoint for network communication. Think of it
// as a "phone" — you can call someone (connect), answer a call (accept),
// talk (send), listen (recv), and hang up (close).
//
// In C, sockets are just integers (file descriptors). If you forget to
// close() them, you LEAK resources — the OS eventually runs out of file
// descriptors and your program can't open new files, sockets, or pipes.
//
// WHAT IS RAII? (Resource Acquisition Is Initialization)
// RAII is THE most important C++ idiom. The idea is simple:
//   - ACQUIRE a resource in the constructor (open a socket)
//   - RELEASE the resource in the destructor (close the socket)
//
// Because destructors run AUTOMATICALLY when an object goes out of scope
// (even if an exception is thrown!), RAII GUARANTEES cleanup. No resource
// leaks.
//
// Examples of RAII in the standard library:
//   - std::lock_guard: acquires mutex in ctor, releases in dtor
//   - std::unique_ptr: allocates memory, deletes in dtor
//   - std::fstream: opens file in ctor, closes in dtor
//   - std::thread: (must join/detach before dtor)
//
// Our Socket class: opens a socket in ctor, closes in dtor. Simple!
// =============================================================================

#pragma once

// System headers for POSIX sockets (Mac/Linux)
// These are C headers (not C++), but they work fine in C++ code.
#include <netinet/in.h> // sockaddr_in (IPv4 address structure)
#include <sys/socket.h> // socket(), bind(), listen(), accept()
#include <unistd.h>     // close() (close file descriptors)

#include <optional>
#include <string>

namespace mini_redis {

class Socket {
public:
  // ---- Constructor: create a new socket ----
  // "default" means "use the compiler-generated version."
  // We provide a factory function (create_tcp) instead of making
  // the constructor do the work. This is the FACTORY PATTERN — the
  // constructor just initializes an empty state, and a separate
  // function does the actual creation (and can return errors cleanly).
  Socket() = default;

  // ---- Destructor: close the socket ----
  // Automatically called when the Socket object is destroyed.
  // This is the "cleanup" guarantee of RAII.
  ~Socket();

  // ---- Move operations ----
  // WHAT IS MOVE SEMANTICS?
  // In competitive programming, you never think about what happens
  // when you do: vector<int> a = b; — it COPIES all elements.
  //
  // MOVE semantics (C++11) let you TRANSFER ownership instead of copying:
  //   Socket a = std::move(b);  // a takes b's socket, b becomes empty
  //
  // This is critical for resources like sockets, files, threads.
  // You can't COPY a socket (what would it even mean to have two copies
  // of the same network connection?), but you CAN MOVE it.
  //
  // && is an "rvalue reference" — it means "I accept temporary objects
  // or explicitly moved objects." The compiler uses this to distinguish
  // between copies (const Socket&) and moves (Socket&&).
  //
  // "noexcept" tells the compiler this function will NOT throw exceptions.
  // This enables optimizations: std::vector can use move instead of copy
  // when reallocating, but ONLY if move is noexcept (otherwise, if move
  // threw halfway through, the vector would be in a corrupted state).
  Socket(Socket &&other) noexcept;
  Socket &operator=(Socket &&other) noexcept;

  // ---- Delete copy operations ----
  // Copying a socket makes NO sense — two objects would try to close()
  // the same file descriptor, causing a double-close bug.
  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  // ---- Factory: create a TCP socket ----
  // Returns a configured Socket, or std::nullopt on failure.
  //
  // WHY A FACTORY INSTEAD OF A CONSTRUCTOR?
  // Constructors CAN'T return error codes. If construction fails, you
  // have to throw an exception (which can be expensive and hard to handle).
  // A static factory function can return std::optional — clean and explicit.
  //
  // "static" means you call it as Socket::create_tcp(), not socket.create_tcp()
  static std::optional<Socket> create_tcp();

  // ---- Bind to a port ----
  // "Bind" means "associate this socket with a specific port on this machine."
  // After binding to port 8080, incoming connections to port 8080 reach this
  // socket.
  bool bind_to(int port);

  // ---- Start listening for connections ----
  // After bind, call listen() to tell the OS "I'm ready to accept connections."
  // backlog = how many pending connections to queue before rejecting new ones.
  bool start_listening(int backlog = 128);

  // ---- Accept an incoming connection ----
  // Blocks until a client connects, then returns a NEW Socket for that
  // connection. The original (listening) socket continues accepting more
  // connections.
  std::optional<Socket> accept_connection();

  // ---- Read data from the socket ----
  // Returns the data as a string, or empty string on error/disconnect.
  std::string read_all();

  // ---- Write data to the socket ----
  // Returns true if all bytes were sent successfully.
  bool write_all(const std::string &data);

  // ---- Get the raw file descriptor (for logging/debugging) ----
  int file_descriptor() const;

private:
  // ---- Private constructor from an existing file descriptor ----
  // Used internally by accept_connection() which gets an fd from the OS.
  explicit Socket(int fd);

  // ---- Close the socket (internal helper) ----
  void close_socket();

  // The raw OS file descriptor.
  // -1 means "no socket" (invalid/closed).
  // File descriptors are small non-negative integers that the OS uses
  // to identify open files, sockets, pipes, etc.
  int fd_ = -1;
};

} // namespace mini_redis
