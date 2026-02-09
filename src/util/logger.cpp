// =============================================================================
// logger.cpp — Thread-Safe Logging Utility (IMPLEMENTATION FILE)
// =============================================================================
//
// This .cpp file contains the DEFINITIONS (actual code) for the functions
// that were DECLARED (promised) in logger.hpp.
//
// RULE: The header (.hpp) says "this function exists."
//       The source (.cpp) says "here's what it actually does."
// =============================================================================

// Include our own header first. This is a convention:
// 1. Own header first (catches missing includes in the header)
// 2. Standard library headers
// 3. Third-party library headers
#include "util/logger.hpp"

// <iostream> provides std::cout and std::endl for printing to the terminal.
// We include it HERE (not in the header) because only the .cpp needs it.
// This is called "minimizing header dependencies" — the fewer includes in
// your header, the faster your project compiles.
#include <iostream>

// <chrono> provides time utilities — we use it to get the current time
// for timestamps in log messages.
#include <chrono>

// <iomanip> provides std::put_time for formatting time as strings
#include <iomanip>

// <sstream> provides std::ostringstream — a string builder that works
// like cout but writes to a string instead of the screen
#include <sstream>

namespace mini_redis {

// =============================================================================
// Logger::info — Log an informational message
// =============================================================================
// "const std::string&" explained:
//   - const   : we promise NOT to modify the string (read-only)
//   - std::string : the string type
//   - &       : "reference" — means we DON'T copy the string, we just
//               borrow it. Copying a string allocates memory, which is slow.
//
// In competitive programming, you'd write: void info(string message)
// This COPIES the string every time — wasteful for a function that just reads
// it. In industry, we pass by const reference to avoid unnecessary copies.
// =============================================================================
void Logger::info(const std::string &message) {
  // Delegate to the private log() function with INFO level
  log(LogLevel::INFO, message);
}

void Logger::warning(const std::string &message) {
  log(LogLevel::WARNING, message);
}

void Logger::error(const std::string &message) {
  log(LogLevel::ERROR, message);
}

// =============================================================================
// Logger::log — The core logging function (private)
// =============================================================================
// This is where the actual writing happens. It's private because external
// code should use info() / warning() / error() instead.
// =============================================================================
void Logger::log(LogLevel level, const std::string &message) {
  // --- Thread safety with std::lock_guard ---
  // PROBLEM: if two threads call log() at the same time, their output
  // can interleave: "[INFO] Hel[ERROR] Failed to blo world"
  //
  // SOLUTION: lock_guard acquires the mutex in its CONSTRUCTOR and
  // releases it in its DESTRUCTOR (when it goes out of scope).
  //
  // WHAT IS A DESTRUCTOR?
  // A destructor is a special function that runs automatically when an
  // object is destroyed (goes out of scope). It's the opposite of a
  // constructor. Syntax: ~ClassName()
  //
  // For lock_guard:  constructor → locks the mutex
  //                  destructor  → unlocks the mutex
  //
  // This pattern is called RAII (Resource Acquisition Is Initialization):
  // "Acquire the resource (lock) when the object is created,
  //  release the resource (unlock) when the object is destroyed."
  //
  // Why is RAII amazing? Even if an EXCEPTION is thrown between the
  // lock and unlock, the destructor STILL runs, so the mutex is
  // ALWAYS unlocked. No deadlocks from forgotten unlocks!
  std::lock_guard<std::mutex> lock(log_mutex_);

  // Build the log line and print it
  std::cout << "[" << current_timestamp() << "] "
            << "[" << level_to_string(level) << "] " << message
            << std::endl; // endl flushes the buffer (ensures immediate print)
}

// =============================================================================
// Logger::level_to_string — Convert enum to display string
// =============================================================================
std::string Logger::level_to_string(LogLevel level) {
  // "switch" on an enum class — the compiler warns if you forget a case!
  switch (level) {
  case LogLevel::INFO:
    return "INFO   ";
  case LogLevel::WARNING:
    return "WARNING";
  case LogLevel::ERROR:
    return "ERROR  ";
  }

  // This line should never be reached if all enum cases are covered.
  // But the compiler doesn't always know that, so we add a fallback.
  return "UNKNOWN";
}

// =============================================================================
// Logger::current_timestamp — Get current time as "YYYY-MM-DD HH:MM:SS"
// =============================================================================
std::string Logger::current_timestamp() {
  // Get the current time from the system clock
  const auto now = std::chrono::system_clock::now();

  // Convert to time_t (old-style C time) for formatting
  const auto time_t_now = std::chrono::system_clock::to_time_t(now);

  // Convert to local time struct (tm = "time structure")
  // std::localtime returns a pointer to a static buffer — NOT thread-safe
  // in theory, but our log_mutex_ protects this entire function
  const auto *local_time = std::localtime(&time_t_now);

  // Use ostringstream to build the formatted string
  std::ostringstream stream;
  stream << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");

  // .str() extracts the string from the stream
  return stream.str();
}

} // namespace mini_redis
