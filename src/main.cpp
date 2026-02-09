// =============================================================================
// main.cpp — Entry Point
// =============================================================================
//
// This is where the program starts. It's deliberately SHORT — its only
// job is to create the Application and run it. All the real logic lives
// in the other classes.
//
// SIGNAL HANDLING:
// When you press Ctrl+C in the terminal, the OS sends a SIGINT signal
// to your program. Without handling it, the program is killed immediately,
// which can leave resources in a dirty state (open files, half-written data).
//
// We install a signal handler that catches SIGINT and calls app.stop()
// for a GRACEFUL SHUTDOWN — closing connections, flushing logs, etc.
//
// WHY IS THE SIGNAL HANDLER APPROACH TRICKY?
// Signal handlers run in a special context with severe restrictions:
//   - Can't use most standard library functions (no cout, no new, no mutex)
//   - Can only safely write to volatile sig_atomic_t variables
//   - Must be as short as possible
//
// Our approach: set a global atomic flag in the handler, check it in the main
// loop.
// =============================================================================

#include "app/application.hpp"
#include "util/logger.hpp"

// <csignal> provides signal() and SIGINT
// <atomic> provides std::atomic for the signal flag
#include <atomic>
#include <csignal>

// =============================================================================
// Global signal flag
// =============================================================================
// WHY GLOBAL?
// Signal handlers are C-style function pointers — they can't capture context
// (unlike lambdas). So we use a global variable that both the signal handler
// and main() can access.
//
// std::atomic_bool is thread-safe for simple reads/writes without a mutex.
// We use a global pointer to the application so the signal handler can
// call stop() on it.
// =============================================================================
namespace {

// Global pointer to the running application (for signal handler access)
// We use a raw pointer here (not smart pointer) because:
//   - The application lives on the stack in main()
//   - We're not transferring ownership
//   - Signal handlers can't use complex types anyway
mini_redis::Application *g_app = nullptr;

// The signal handler function — called when Ctrl+C is pressed
// "sig" is the signal number (SIGINT = 2)
void signal_handler(int /*sig*/) {
  // Set the stop flag — the accept loop will check this and exit
  if (g_app != nullptr) {
    g_app->stop();
  }
}

} // anonymous namespace

// =============================================================================
// main() — Program entry point
// =============================================================================
// In C++, main() is the first function called by the runtime.
// It returns an int: 0 = success, non-zero = error.
//
// argc = argument count (number of command-line arguments)
// argv = argument vector (array of C-strings)
//
// For now, we hardcode port 8080 and 4 threads.
// A production app would use a config file or command-line arguments.
// =============================================================================
int main() {
  // Install the SIGINT handler (Ctrl+C)
  // std::signal(signal_number, handler_function) returns the previous handler
  std::signal(SIGINT, signal_handler);

  mini_redis::Logger::info("Starting Mini Redis...");

  // Create the application on the STACK (not the heap).
  // Stack allocation is faster than heap allocation (new/delete).
  // Since the app lives for the entire program, stack is perfect.
  //
  // PORT 8080: a common port for development HTTP servers.
  //   - Ports below 1024 require root/admin privileges
  //   - Ports 1024-65535 can be used by any program
  //   - 8080 is the conventional "alternative HTTP" port
  //
  // 4 THREADS: one per CPU core is a good default.
  //   - More threads than cores = context switching overhead
  //   - Fewer threads than cores = underutilization
  mini_redis::Application app(8080, 4);

  // Set the global pointer so the signal handler can access it
  g_app = &app;

  // Run the application (blocks until Ctrl+C)
  app.run();

  mini_redis::Logger::info("Mini Redis exited cleanly");

  // Return 0 to indicate successful execution to the OS
  return 0;
}
