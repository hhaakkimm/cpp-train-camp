// =============================================================================
// thread_pool.hpp — Fixed-Size Thread Pool (HEADER)
// =============================================================================
//
// WHAT IS A THREAD POOL?
// Imagine a restaurant with 4 waiters (threads). When a customer (task)
// arrives, an available waiter handles them. If all waiters are busy, the
// customer waits in a queue. This is more efficient than hiring a new waiter
// for EVERY customer (creating a new thread per request), because:
//
// 1. Creating/destroying threads is expensive (the OS has to allocate stack
//    memory, set up registers, etc.)
// 2. Too many threads cause "context switching" overhead — the CPU spends
//    more time switching between threads than doing actual work
// 3. A thread pool reuses existing threads, amortizing the creation cost
//
// PATTERN: Producer-Consumer
//   - Producer: the server accept loop pushes tasks into the queue
//   - Consumers: pool threads pop tasks from the queue and execute them
// =============================================================================

#pragma once

#include <atomic>             // std::atomic — lock-free thread-safe variables
#include <condition_variable> // std::condition_variable — wait/notify mechanism
#include <functional>         // std::function — type-erased callable wrapper
#include <mutex>              // std::mutex — mutual exclusion lock
#include <queue>              // std::queue — FIFO queue
#include <thread>             // std::thread — OS-level thread
#include <vector>             // std::vector — dynamic array

namespace mini_redis {

// =============================================================================
// WHAT IS std::function<void()>?
// std::function is a "type-erased callable wrapper". In simple terms:
// it's a variable that can hold ANY callable thing — a regular function,
// a lambda, a functor — as long as it matches the signature.
//
// std::function<void()> means: "a callable that takes no arguments and
// returns nothing." This is our "task" type — any unit of work.
//
// WHAT IS A LAMBDA?
// A lambda is an anonymous (unnamed) function defined inline:
//   auto greet = [](std::string name) { std::cout << "Hi " << name; };
//   greet("Alice");  // prints "Hi Alice"
//
// The [] is the "capture list" — it lists variables from the surrounding
// scope that the lambda needs access to:
//   [x]      — capture x by copy
//   [&x]     — capture x by reference
//   [=]      — capture everything by copy
//   [&]      — capture everything by reference
//   [this]   — capture the current object's pointer
// =============================================================================
using Task = std::function<void()>;

class ThreadPool {
public:
  // ---- Constructor ----
  // "explicit" prevents accidental implicit conversions.
  // Without explicit: ThreadPool pool = 4;  ← compiles but is confusing!
  // With explicit:    ThreadPool pool(4);    ← clear and intentional
  //
  // size_t is an unsigned integer type used for sizes/counts.
  // It's guaranteed to be large enough to represent the size of any object.
  explicit ThreadPool(std::size_t num_threads);

  // ---- Destructor ----
  // Called automatically when the ThreadPool object is destroyed.
  // It stops all worker threads and waits for them to finish (join).
  //
  // WHAT IS ~ (tilde)?
  // ~ClassName() is the destructor syntax. The ~ means "opposite of" the
  // constructor. Constructor sets things up, destructor tears them down.
  ~ThreadPool();

  // ---- Delete copy operations ----
  // WHAT DOES "= delete" MEAN?
  // It tells the compiler: "this function is FORBIDDEN — don't generate
  // it, and give a compile error if anyone tries to use it."
  //
  // WHY delete copy for ThreadPool?
  // Copying a thread pool makes no sense — you'd have two pools trying
  // to manage the same threads. This is a RESOURCE-OWNING class, and
  // resources should not be silently duplicated.
  //
  // In competitive programming, you never think about this because you
  // rarely create classes that own OS resources. In industry, this is
  // critical for preventing bugs.
  //
  // THE RULE OF FIVE:
  // If your class defines ANY of these 5, it should define ALL of them:
  //   1. Destructor
  //   2. Copy constructor
  //   3. Copy assignment operator
  //   4. Move constructor
  //   5. Move assignment operator
  // We define the destructor and delete the rest, because ThreadPool
  // should be neither copied nor moved.
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;

  // ---- Submit a task to the pool ----
  // Takes a Task (std::function<void()>) and adds it to the work queue.
  // A waiting worker thread will pick it up and execute it.
  void submit(Task task);

private:
  // Worker threads — each one runs a loop: wait for task → execute → repeat
  std::vector<std::thread> workers_;

  // The task queue — thread-safe FIFO queue protected by a mutex
  std::queue<Task> task_queue_;

  // Mutex protects the task_queue_ from concurrent access
  std::mutex queue_mutex_;

  // Condition variable — lets workers SLEEP until a task is available,
  // instead of busy-waiting (spinning in a loop checking the queue).
  //
  // Busy-waiting:     while (queue.empty()) {}  ← wastes 100% CPU
  // Condition var:    cv.wait(lock, pred);       ← sleeps, uses 0% CPU
  std::condition_variable condition_;

  // Atomic boolean — signals workers to stop.
  // "atomic" means reads/writes to this variable are thread-safe WITHOUT
  // needing a mutex. This is faster than a mutex for simple flags.
  std::atomic<bool> stop_requested_{false};

  // The function each worker thread runs — defined in the .cpp
  void worker_loop();
};

} // namespace mini_redis
