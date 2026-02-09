// =============================================================================
// socket.cpp — RAII Socket Wrapper (IMPLEMENTATION)
// =============================================================================

#include "network/socket.hpp"
#include "util/logger.hpp"

#include <array>   // std::array — fixed-size array (safer than C arrays)
#include <cstring> // std::memset — fill memory with zeros

namespace mini_redis {

// =============================================================================
// BUFFER_SIZE — How many bytes to read at once
// =============================================================================
// "constexpr" means "constant expression" — evaluated at COMPILE TIME.
// Unlike "const" (which means "can't change at runtime"), constexpr means
// the value is known to the COMPILER and can be used in array sizes,
// template arguments, etc.
//
// WHY NOT "#define BUFFER_SIZE 4096"?
// #define is a C-style macro — the preprocessor does text replacement
// before the compiler even sees it. Problems:
//   - No type checking (it's just text)
//   - No scope (it's global — can conflict with other names)
//   - Doesn't appear in debuggers
// constexpr is a proper typed constant with scope and type safety.
// =============================================================================
constexpr std::size_t BUFFER_SIZE = 4096;

// =============================================================================
// Private constructor — wrap an existing file descriptor
// =============================================================================
Socket::Socket(int fd) : fd_(fd) {
  // Just stores the file descriptor. The OS already created the socket.
}

// =============================================================================
// Destructor — close the socket
// =============================================================================
// This is the RAII cleanup. When a Socket object dies (goes out of scope,
// is explicitly deleted, or the program exits), this runs automatically
// and closes the OS socket. No resource leaks, guaranteed.
// =============================================================================
Socket::~Socket() { close_socket(); }

// =============================================================================
// Move constructor — transfer ownership from another Socket
// =============================================================================
// HOW MOVE WORKS STEP BY STEP:
// Before:  other.fd_ = 5,  this.fd_ = (uninitialized)
// After:   other.fd_ = -1, this.fd_ = 5
//
// We "steal" the file descriptor and set the source to -1 (invalid).
// When the source's destructor runs, it sees fd_ = -1 and does nothing.
// When OUR destructor runs, it closes fd_ = 5. Only ONE close happens!
//
// std::exchange(a, b) atomically:
//   1. Returns the OLD value of a (5)
//   2. Sets a to b (-1)
// This is cleaner than: this->fd_ = other.fd_; other.fd_ = -1;
// =============================================================================
Socket::Socket(Socket &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {
  // all done in the initializer list
}

// =============================================================================
// Move assignment — transfer ownership from another Socket
// =============================================================================
// This handles the case: existing_socket = std::move(other_socket);
// We must close our CURRENT socket first (if any), then steal the other's.
// =============================================================================
Socket &Socket::operator=(Socket &&other) noexcept {
  // Self-assignment check: a = std::move(a) — do nothing
  if (this != &other) {
    // Close our current socket (if we have one)
    close_socket();

    // Steal the other socket's file descriptor
    fd_ = std::exchange(other.fd_, -1);
  }

  // Return *this to allow chaining: a = b = std::move(c);
  // "this" is a pointer to the current object.
  // "*this" dereferences it to get the object itself.
  return *this;
}

// =============================================================================
// create_tcp() — Factory: create a new TCP socket
// =============================================================================
std::optional<Socket> Socket::create_tcp() {
  // socket() system call creates a new socket.
  //   AF_INET     = IPv4 (Internet Protocol version 4)
  //   SOCK_STREAM = TCP (reliable, ordered byte stream — like a pipe)
  //   0           = let the OS choose the protocol (TCP for SOCK_STREAM)
  //
  // Returns a file descriptor (non-negative int) on success, or -1 on error.
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);

  // :: (scope resolution operator with nothing before it) means
  // "call the GLOBAL socket() function" (the OS system call),
  // not any member function named socket() in our class.
  if (fd < 0) {
    Logger::error("Failed to create TCP socket");
    return std::nullopt;
  }

  // Enable SO_REUSEADDR — allows binding to a port that was recently used.
  // Without this, after stopping the server, you'd have to wait ~60 seconds
  // before restarting (the OS keeps the port in TIME_WAIT state).
  const int opt = 1;
  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    Logger::warning("Failed to set SO_REUSEADDR");
    // Not fatal — continue anyway
  }

  // Return a Socket object wrapping this fd
  // The Socket(int) constructor is called here (it's private, but
  // we're inside the Socket class, so we can call private members).
  return Socket(fd);
}

// =============================================================================
// bind_to() — Associate this socket with a specific port
// =============================================================================
bool Socket::bind_to(int port) {
  // sockaddr_in is the IPv4 address structure. We need to fill it in:
  //   sin_family : AF_INET (IPv4)
  //   sin_addr   : which IP to listen on (INADDR_ANY = all interfaces)
  //   sin_port   : port number (must be in "network byte order")
  struct sockaddr_in address{}; // {} = zero-initialize all fields

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces

  // htons = "host to network short" — converts port number to big-endian.
  // Networks use big-endian byte order, but your CPU might use little-endian.
  // htons() handles the conversion portably.
  address.sin_port = htons(static_cast<uint16_t>(port));

  // static_cast<type>(value) is the C++ way to convert types explicitly.
  // Unlike C-style cast "(int)x", static_cast is:
  //   - Checked at compile time (won't let you do dangerous casts)
  //   - Searchable (grep for "static_cast" to find all conversions)
  //   - Self-documenting (tells the reader this is an intentional conversion)

  // bind() system call associates the socket with the address
  // reinterpret_cast is needed because bind() takes a generic sockaddr*
  // but we have a sockaddr_in*. This is a C legacy — the network API
  // predates C++, so it uses void-like generic pointers.
  const int result = ::bind(fd_, reinterpret_cast<struct sockaddr *>(&address),
                            sizeof(address));

  if (result < 0) {
    Logger::error("Failed to bind to port " + std::to_string(port));
    return false;
  }

  Logger::info("Socket bound to port " + std::to_string(port));
  return true;
}

// =============================================================================
// start_listening() — Mark socket as ready to accept connections
// =============================================================================
bool Socket::start_listening(int backlog) {
  // listen() tells the OS: "I want to accept incoming connections."
  // backlog = max number of pending connections in the queue.
  // If the queue is full, new connection attempts are rejected.
  const int result = ::listen(fd_, backlog);

  if (result < 0) {
    Logger::error("Failed to start listening");
    return false;
  }

  return true;
}

// =============================================================================
// accept_connection() — Wait for and accept a new client connection
// =============================================================================
std::optional<Socket> Socket::accept_connection() {
  // accept() blocks until a client connects, then returns a NEW file
  // descriptor for the client connection. The original socket continues
  // listening for more connections.
  //
  // We pass nullptr for the client address — we don't need to know
  // who connected (for simplicity). In production, you might log the
  // client's IP address for security/monitoring.
  const int client_fd = ::accept(fd_, nullptr, nullptr);

  if (client_fd < 0) {
    Logger::error("Failed to accept connection");
    return std::nullopt;
  }

  return Socket(client_fd);
}

// =============================================================================
// read_all() — Read all available data from the socket
// =============================================================================
std::string Socket::read_all() {
  // std::array is a fixed-size array that knows its own size.
  // Unlike C arrays (char buf[4096]):
  //   - Doesn't decay to a pointer
  //   - Has .size(), .data(), .begin(), .end()
  //   - Can be passed by value (though you usually pass by reference)
  std::array<char, BUFFER_SIZE> buffer{}; // Zero-initialized

  // recv() reads up to buffer.size() bytes from the socket.
  // Returns the number of bytes actually read, 0 on disconnect, -1 on error.
  const ssize_t bytes_read = ::recv(fd_, buffer.data(), buffer.size(), 0);

  if (bytes_read <= 0) {
    return ""; // Error or client disconnected
  }

  // Construct a string from the buffer, using only the bytes we actually read
  return std::string(buffer.data(), static_cast<std::size_t>(bytes_read));
}

// =============================================================================
// write_all() — Send all data through the socket
// =============================================================================
bool Socket::write_all(const std::string &data) {
  // Total number of bytes to send
  std::size_t total_sent = 0;
  const std::size_t data_size = data.size();

  // We loop because send() might not send ALL bytes at once.
  // This is a common networking pitfall — the OS might only accept
  // part of your data if its internal buffer is full.
  while (total_sent < data_size) {
    // .c_str() returns a const char* (C-style string pointer)
    // We offset it by total_sent to continue from where we left off
    const ssize_t bytes_sent =
        ::send(fd_, data.c_str() + total_sent, data_size - total_sent,
               0 // flags (0 = no special behavior)
        );

    if (bytes_sent < 0) {
      Logger::error("Failed to send data");
      return false;
    }

    total_sent += static_cast<std::size_t>(bytes_sent);
  }

  return true;
}

// =============================================================================
// file_descriptor() — Getter for the raw fd (for debugging)
// =============================================================================
int Socket::file_descriptor() const { return fd_; }

// =============================================================================
// close_socket() — Close the file descriptor
// =============================================================================
void Socket::close_socket() {
  // Only close if we have a valid file descriptor
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1; // Mark as invalid so we don't close it again
  }
}

} // namespace mini_redis
