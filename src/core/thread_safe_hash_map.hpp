// =============================================================================
// thread_safe_hash_map.hpp — Concurrent Hash Map with Reader-Writer Lock
// =============================================================================
//
// WHAT IS THIS?
// A wrapper around std::unordered_map that is SAFE to use from multiple
// threads simultaneously. Without thread safety, two threads modifying
// the map at the same time causes UNDEFINED BEHAVIOR (crashes, corrupted
// data, or silently wrong results — the worst kind of bug).
//
// KEY CONCEPT: std::shared_mutex (Reader-Writer Lock)
// Regular mutex: only ONE thread can hold it at a time.
// shared_mutex: TWO modes:
//   - shared lock (read mode):    MULTIPLE threads can hold it simultaneously
//   - exclusive lock (write mode): only ONE thread can hold it, and NO readers
//
// WHY DOES THIS MATTER?
// A key-value store is typically read-heavy (many GETs, fewer SETs).
// With a regular mutex, GETs would block each other — totally unnecessary
// since reading doesn't modify data. With shared_mutex, 100 GET requests
// can proceed in parallel, only blocking when a SET/DELETE needs to write.
//
// WHAT IS A TEMPLATE?
// Templates let you write code that works with ANY type:
//   ThreadSafeHashMap<std::string, int>    → map from string to int
//   ThreadSafeHashMap<int, std::string>    → map from int to string
//
// The compiler generates a separate version of the class for each type
// combination you use. This is called "template instantiation."
//
// In competitive programming, you use templates without thinking (vector<int>).
// Here, we're CREATING a template class.
// =============================================================================

#pragma once

#include <functional>    // std::function — for callbacks
#include <optional>      // std::optional — a value that might not exist
#include <shared_mutex>  // std::shared_mutex — reader-writer lock
#include <string>        // std::string
#include <unordered_map> // The underlying hash table
#include <vector>        // std::vector — dynamic array

namespace mini_redis {

// =============================================================================
// WHAT IS std::optional<T>?
// In competitive programming, you might return -1 or "" to mean "not found."
// That's fragile — what if -1 is a valid value?
//
// std::optional<T> is a container that either holds a value of type T, or
// holds NOTHING (std::nullopt). It makes "no value" explicit and type-safe:
//
//   std::optional<int> find(int key) {
//       if (found) return 42;        // has a value
//       else return std::nullopt;    // explicitly "no value"
//   }
//
//   auto result = find(5);
//   if (result.has_value()) {        // or just: if (result)
//       std::cout << result.value(); // or: *result
//   }
// =============================================================================

template <typename Key, typename Value> class ThreadSafeHashMap {
public:
  // ---- get() — Thread-safe read ----
  // Returns std::optional<Value>:
  //   - If the key exists → returns the value wrapped in optional
  //   - If the key doesn't exist → returns std::nullopt
  //
  // "const" after the parameter list means this function does NOT modify
  // the object. This is a PROMISE to the compiler and the caller.
  // The compiler enforces it — you'll get an error if you try to modify
  // any member variables inside a const function.
  std::optional<Value> get(const Key &key) const;

  // ---- set() — Thread-safe write ----
  // Inserts or overwrites the value for the given key.
  void set(const Key &key, const Value &value);

  // ---- remove() — Thread-safe delete ----
  // Returns true if the key was found and removed, false if it didn't exist.
  bool remove(const Key &key);

  // ---- keys() — Get all keys (thread-safe) ----
  // Returns a COPY of all keys. Returning by value (not by reference)
  // is intentional: the caller gets their own copy that won't be
  // invalidated if another thread modifies the map.
  std::vector<Key> keys() const;

  // ---- size() — Number of entries ----
  std::size_t size() const;

  // ---- for_each() — Iterate with a callback (thread-safe) ----
  // Takes a function that receives (key, value) for each entry.
  // The entire iteration happens under a shared lock, so the map
  // won't change mid-iteration.
  //
  // WHY USE A CALLBACK INSTEAD OF RETURNING AN ITERATOR?
  // Iterators become invalid if another thread modifies the map.
  // By running the callback under our lock, we guarantee consistency.
  void for_each(
      const std::function<void(const Key &, const Value &)> &callback) const;

  // ---- remove_if() — Remove entries matching a condition ----
  // Takes a predicate function. For each entry where predicate returns
  // true, that entry is removed. Returns how many entries were removed.
  // Used by the expiry manager to remove expired keys.
  std::size_t
  remove_if(const std::function<bool(const Key &, const Value &)> &predicate);

private:
  // The actual data — a standard hash map
  std::unordered_map<Key, Value> map_;

  // "mutable" keyword explained:
  // Problem: get() is a const function (doesn't modify the map data),
  // but it needs to LOCK the mutex (which modifies the mutex's state).
  //
  // "mutable" says: "this member CAN be modified even in const functions."
  // It's used for synchronization primitives and caches — things that
  // are implementation details, not observable state.
  //
  // Without mutable: the const get() function can't lock the mutex → won't
  // compile With mutable:    the const get() function can lock/unlock the mutex
  // ✓
  mutable std::shared_mutex mutex_;
};

// =============================================================================
// TEMPLATE IMPLEMENTATION
// =============================================================================
// IMPORTANT: Template implementations MUST be in the HEADER file (or a separate
// .hpp included by the header). They CANNOT go in a .cpp file.
//
// WHY? When the compiler sees ThreadSafeHashMap<string, string>, it needs to
// generate code for that specific type combination. It can only do that if it
// can see the full implementation. If the implementation is in a .cpp file,
// the compiler can't see it when compiling OTHER .cpp files that use the
// template.
//
// This is the ONE exception to the "implementations go in .cpp" rule.
// =============================================================================

template <typename Key, typename Value>
std::optional<Value> ThreadSafeHashMap<Key, Value>::get(const Key &key) const {
  // shared_lock = READ lock — multiple threads can hold this simultaneously
  // This is safe because reading doesn't modify the map.
  std::shared_lock<std::shared_mutex> lock(mutex_);

  // .find() returns an iterator to the element, or .end() if not found
  const auto it = map_.find(key);

  if (it == map_.end()) {
    // Key not found → return "empty" optional
    return std::nullopt;
  }

  // Key found → return the value wrapped in optional
  // it->second is the value (it->first would be the key)
  return it->second;
}

template <typename Key, typename Value>
void ThreadSafeHashMap<Key, Value>::set(const Key &key, const Value &value) {
  // unique_lock (or lock_guard) = EXCLUSIVE/WRITE lock
  // Only ONE thread can hold this. All readers and writers must wait.
  std::lock_guard<std::shared_mutex> lock(mutex_);

  // insert_or_assign: if key exists → overwrite; if not → insert
  // This is cleaner than map_[key] = value because:
  //   - map_[key] requires Value to be default-constructible
  //   - insert_or_assign works with any type
  map_.insert_or_assign(key, value);
}

template <typename Key, typename Value>
bool ThreadSafeHashMap<Key, Value>::remove(const Key &key) {
  std::lock_guard<std::shared_mutex> lock(mutex_);

  // erase() returns the number of elements removed (0 or 1 for maps)
  return map_.erase(key) > 0;
}

template <typename Key, typename Value>
std::vector<Key> ThreadSafeHashMap<Key, Value>::keys() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);

  std::vector<Key> result;
  result.reserve(map_.size()); // Pre-allocate to avoid reallocations

  for (const auto &[key, value] : map_) {
    // ^^^^^^^^^^^^^^^^^^^^^^^^
    // STRUCTURED BINDINGS (C++17)
    // This unpacks each map entry into "key" and "value" variables.
    // Before C++17, you'd write: for (const auto& pair : map_) { pair.first; }
    // Structured bindings are cleaner and more readable.
    result.push_back(key);
  }

  return result;
  // NOTE: returning a local variable triggers NRVO (Named Return Value
  // Optimization). The compiler constructs the vector directly in the
  // caller's memory — NO copy happens. Modern C++ is smart about this.
}

template <typename Key, typename Value>
std::size_t ThreadSafeHashMap<Key, Value>::size() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return map_.size();
}

template <typename Key, typename Value>
void ThreadSafeHashMap<Key, Value>::for_each(
    const std::function<void(const Key &, const Value &)> &callback) const {

  std::shared_lock<std::shared_mutex> lock(mutex_);

  for (const auto &[key, value] : map_) {
    callback(key, value);
  }
}

template <typename Key, typename Value>
std::size_t ThreadSafeHashMap<Key, Value>::remove_if(
    const std::function<bool(const Key &, const Value &)> &predicate) {

  // Exclusive lock because we're modifying the map
  std::lock_guard<std::shared_mutex> lock(mutex_);

  std::size_t removed_count = 0;

  // We can't use range-for here because we're erasing during iteration.
  // Erasing invalidates the iterator, so we use the "erase and advance"
  // pattern:
  for (auto it = map_.begin(); it != map_.end(); /* no increment here */) {
    if (predicate(it->first, it->second)) {
      // erase() returns an iterator to the NEXT element
      it = map_.erase(it);
      ++removed_count;
    } else {
      // Only advance if we didn't erase (erase already advances)
      ++it;
    }
  }

  return removed_count;
}

} // namespace mini_redis
