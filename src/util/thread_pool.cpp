// =============================================================================
// thread_pool.cpp — Fixed-Size Thread Pool (IMPLEMENTATION)
// =============================================================================

#include "util/thread_pool.hpp"
#include "util/logger.hpp"

#include <string> // for std::to_string

namespace mini_redis {

// =============================================================================
// Constructor — Create worker threads
// =============================================================================
// The constructor creates 'num_threads' worker threads. Each thread runs
// the worker_loop() function, which waits for tasks in a loop.
//
// WHAT ARE INITIALIZER LISTS?
// The ": stop_requested_(false)" syntax is a MEMBER INITIALIZER LIST.
// It initializes member variables BEFORE the constructor body runs.
// This is more efficient than assigning inside the body because:
//   - In the body: the member is default-constructed, then assigned (2 steps)
//   - In the list: the member is directly constructed with the value (1 step)
//
// For primitives like bool, it doesn't matter much. But for complex types
// like std::string or std::vector, it avoids an unnecessary default
// construction. Industry standard: ALWAYS use initializer lists.
// =============================================================================
ThreadPool::ThreadPool(std::size_t num_threads) {
  // Reserve space in the vector to avoid reallocations
  // reserve() allocates memory but doesn't create objects
  workers_.reserve(num_threads);

  for (std::size_t i = 0; i < num_threads; ++i) {
    // emplace_back constructs the thread IN-PLACE in the vector
    // (avoids copying the thread object, which is not allowed anyway)
    //
    // &ThreadPool::worker_loop = pointer to the member function
    // "this" = the ThreadPool object that the function belongs to
    // Together: "run this->worker_loop() in a new thread"
    workers_.emplace_back(&ThreadPool::worker_loop, this);
  }

  Logger::info("Thread pool created with " + std::to_string(num_threads) +
               " workers");
}

// =============================================================================
// Destructor — Shut down all worker threads
// =============================================================================
// WHAT HAPPENS WHEN THE THREADPOOL IS DESTROYED?
// 1. Set the stop flag → workers will exit their loops
// 2. Notify all workers → wake them up if they're sleeping
// 3. Join all workers → wait for them to finish current work
//
// WHAT IS join()?
// thread.join() means "block the current thread and WAIT until the other
// thread finishes." Without join(), the program might exit while threads
// are still running → undefined behavior (crashes, corrupted data).
//
// This is RAII: the destructor ensures cleanup happens automatically.
// =============================================================================
ThreadPool::~ThreadPool() {
  // Step 1: Signal all workers to stop
  stop_requested_.store(true);

  // Step 2: Wake up ALL sleeping workers so they can see the stop flag
  condition_.notify_all();

  // Step 3: Wait for each worker to finish
  for (auto &worker : workers_) {
    // joinable() checks if the thread is still running
    if (worker.joinable()) {
      worker.join();
    }
  }

  Logger::info("Thread pool shut down");
}

// =============================================================================
// submit() — Add a task to the work queue
// =============================================================================
// WHAT IS std::move()?
// In competitive programming, when you pass data around, it gets COPIED.
// Moving is like giving someone your notebook vs photocopying it:
//   - Copy: make a duplicate (expensive for large objects)
//   - Move: transfer ownership (the original is left empty, but it's fast)
//
// std::move() doesn't actually move anything — it just CASTS the object
// to an "rvalue reference" (&&), which tells the receiving function:
// "you can steal this object's internal resources (pointers, buffers)."
//
// For std::function, moving avoids copying the captured variables.
// =============================================================================
void ThreadPool::submit(Task task) {
  {
    // Lock the queue while we modify it
    // Note the extra {} braces — this creates a SCOPE.
    // The lock_guard is destroyed at the closing }, releasing the lock
    // BEFORE we call notify_one(). This is more efficient: the notified
    // worker can immediately acquire the lock without waiting.
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.push(std::move(task));
  }

  // Wake up ONE sleeping worker to handle this task
  // notify_one() vs notify_all():
  //   - notify_one(): wakes exactly one thread (efficient for new tasks)
  //   - notify_all(): wakes ALL threads (used for shutdown/broadcast)
  condition_.notify_one();
}

// =============================================================================
// worker_loop() — The function each worker thread runs
// =============================================================================
// Each worker runs this in an infinite loop:
//   1. Lock the queue
//   2. Sleep until there's a task OR we're told to stop
//   3. If stopping and queue is empty → exit
//   4. Otherwise, grab a task, unlock, execute it
//   5. Go back to step 1
// =============================================================================
void ThreadPool::worker_loop() {
  while (true) {
    Task task;

    {
      // std::unique_lock is like lock_guard but MORE FLEXIBLE:
      // - lock_guard: locks in constructor, unlocks in destructor, THAT'S IT
      // - unique_lock: can also be manually unlocked/relocked, and is
      //   REQUIRED by condition_variable::wait() (because wait() needs
      //   to unlock the mutex while sleeping, then relock it on wakeup)
      std::unique_lock<std::mutex> lock(queue_mutex_);

      // wait() does three things atomically:
      // 1. Checks the predicate (the lambda): if true, DON'T wait
      // 2. If predicate is false, UNLOCK the mutex and SLEEP
      // 3. When notified, RELOCK the mutex and check the predicate again
      //
      // The lambda [this] captures "this" pointer so it can access
      // stop_requested_ and task_queue_ from inside the lambda.
      //
      // Predicate: "wake up if stopping OR there's work to do"
      condition_.wait(lock, [this] {
        return stop_requested_.load() || !task_queue_.empty();
      });

      // If we're stopping AND the queue is empty, exit the loop
      if (stop_requested_.load() && task_queue_.empty()) {
        return; // Thread function returns → thread is done
      }

      // Grab the next task from the front of the queue
      task = std::move(task_queue_.front());
      task_queue_.pop();

      // Lock is released here when 'lock' goes out of scope (RAII!)
    }

    // Execute the task OUTSIDE the lock — this is critical!
    // If we held the lock while running the task, no other worker
    // could grab tasks from the queue during that time.
    task();
  }
}

} // namespace mini_redis
