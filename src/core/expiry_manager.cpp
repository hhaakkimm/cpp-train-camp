// =============================================================================
// expiry_manager.cpp — Background Thread for Key Expiration (IMPLEMENTATION)
// =============================================================================

#include "core/expiry_manager.hpp"
#include "util/logger.hpp"

namespace mini_redis {

// =============================================================================
// Constructor
// =============================================================================
// MEMBER INITIALIZER LIST — initializes members before the body runs.
// store_(store) initializes the REFERENCE member to refer to 'store'.
// interval_(interval_seconds) creates a std::chrono::seconds duration.
//
// NOTE: References MUST be initialized in the initializer list — you can't
// assign to a reference after construction (references can't be rebound).
// =============================================================================
ExpiryManager::ExpiryManager(KeyValueStore &store, int interval_seconds)
    : store_(store), interval_(interval_seconds) {
  // Body intentionally empty — all initialization done in the list above.
  // This is the industrial style: use initializer lists for everything.
}

// =============================================================================
// Destructor — ensure the thread is stopped
// =============================================================================
// Even if the user forgets to call stop(), the destructor handles it.
// This is DEFENSIVE PROGRAMMING — anticipate misuse and handle it gracefully.
// =============================================================================
ExpiryManager::~ExpiryManager() { stop(); }

// =============================================================================
// start() — Launch the background cleanup thread
// =============================================================================
void ExpiryManager::start() {
  // Reset the stop flag in case the manager was previously stopped and
  // restarted
  stop_requested_.store(false);

  // Create a new thread that runs cleanup_loop()
  // std::thread constructor:
  //   arg 1: pointer to member function
  //   arg 2: the object to call it on (this = "call it on this ExpiryManager")
  cleanup_thread_ = std::thread(&ExpiryManager::cleanup_loop, this);

  Logger::info("Expiry manager started (interval: " +
               std::to_string(interval_.count()) + "s)");
}

// =============================================================================
// stop() — Signal the thread to stop and wait for it
// =============================================================================
void ExpiryManager::stop() {
  // Only stop if the thread is actually running
  if (!cleanup_thread_.joinable()) {
    return;
  }

  // Signal the thread to stop
  stop_requested_.store(true);

  // Wake up the thread if it's sleeping in wait_for()
  sleep_cv_.notify_all();

  // Wait for the thread to finish
  cleanup_thread_.join();

  Logger::info("Expiry manager stopped");
}

// =============================================================================
// cleanup_loop() — Runs in the background thread
// =============================================================================
// Loop: cleanup → sleep → cleanup → sleep → ...
// Uses condition_variable::wait_for() for interruptible sleep:
//   - Normally sleeps for 'interval_' seconds
//   - But wakes up IMMEDIATELY if stop() is called (via notify_all)
//
// WHY NOT JUST USE std::this_thread::sleep_for()?
// sleep_for() is NOT interruptible! If interval is 60 seconds and you call
// stop(), you'd have to wait up to 60 seconds for the thread to notice.
// With condition_variable, stop() wakes the thread instantly.
// =============================================================================
void ExpiryManager::cleanup_loop() {
  while (!stop_requested_.load()) {
    // Run one cleanup cycle
    store_.cleanup_expired();

    // Sleep for the interval, but wake up immediately if stop is called
    std::unique_lock<std::mutex> lock(sleep_mutex_);

    // wait_for() returns after either:
    //   1. The timeout (interval_) elapses, OR
    //   2. The condition (stop_requested_) becomes true
    // The lambda is checked BEFORE waiting and on each wakeup.
    sleep_cv_.wait_for(lock, interval_,
                       [this] { return stop_requested_.load(); });
  }
}

} // namespace mini_redis
