// =============================================================================
// expiry_manager.hpp — Background Thread for Key Expiration (HEADER)
// =============================================================================
//
// This class runs a BACKGROUND THREAD that periodically scans the store
// and removes expired keys. It complements the lazy deletion in get().
//
// WHY DO WE NEED BOTH LAZY DELETION AND PERIODIC CLEANUP?
// Lazy deletion only removes keys when they're accessed. If a key expires
// but nobody ever reads it, it stays in memory forever — a memory leak!
// The periodic cleanup catches these "forgotten" expired keys.
//
// DESIGN PATTERN: observe how ExpiryManager doesn't OWN the KeyValueStore.
// It holds a REFERENCE to it. This is DEPENDENCY INJECTION:
//   - ExpiryManager says "give me a store to manage" (via constructor)
//   - The caller decides WHICH store to give it
// This makes ExpiryManager testable — in tests, you could give it a mock store.
// =============================================================================

#pragma once

#include "core/key_value_store.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace mini_redis {

class ExpiryManager {
public:
  // Constructor takes a REFERENCE to the KeyValueStore it manages.
  //
  // WHAT IS A REFERENCE (&)?
  // A reference is an ALIAS for an existing object. Unlike a pointer:
  //   - Can't be null (always refers to a valid object)
  //   - Can't be reassigned to refer to a different object
  //   - No need for -> or * syntax — used just like the original
  //
  // KeyValueStore& store  means "store IS the original object, not a copy."
  // If we wrote KeyValueStore store, it would COPY the entire store — wrong!
  //
  // interval_seconds: how often to scan for expired keys (default: 1 second)
  ExpiryManager(KeyValueStore &store, int interval_seconds = 1);

  // Destructor — stops the background thread
  ~ExpiryManager();

  // Non-copyable, non-movable (owns a running thread)
  ExpiryManager(const ExpiryManager &) = delete;
  ExpiryManager &operator=(const ExpiryManager &) = delete;
  ExpiryManager(ExpiryManager &&) = delete;
  ExpiryManager &operator=(ExpiryManager &&) = delete;

  // Start the background cleanup thread
  void start();

  // Stop the background cleanup thread (waits for it to finish)
  void stop();

private:
  // The function the background thread runs
  void cleanup_loop();

  // Reference to the store we're managing (NOT owned by us)
  KeyValueStore &store_;

  // How often to run cleanup (in seconds)
  std::chrono::seconds interval_;

  // The background thread itself
  std::thread cleanup_thread_;

  // Flag to signal the thread to stop
  std::atomic<bool> stop_requested_{false};

  // Used for timed waiting (sleep between cleanup cycles, but wake
  // up immediately when stop() is called)
  std::mutex sleep_mutex_;
  std::condition_variable sleep_cv_;
};

} // namespace mini_redis
