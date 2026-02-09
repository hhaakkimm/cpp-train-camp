// =============================================================================
// key_value_store.hpp — High-Level Key-Value Store (HEADER)
// =============================================================================
//
// This is the APPLICATION-LEVEL storage. It builds on top of
// ThreadSafeHashMap and adds:
//   1. TTL (Time-To-Live) — keys can expire after a set number of seconds
//   2. A StoreEntry struct that holds both the value and expiration time
//
// DESIGN PRINCIPLE: Single Responsibility (the "S" in SOLID)
// ThreadSafeHashMap handles thread-safe data access.
// KeyValueStore handles the business logic (expiration, key validation).
// Each class has ONE reason to change — THIS is how industrial code is
// organized.
//
// WHAT IS SOLID?
// SOLID is 5 design principles for maintainable code:
//   S — Single Responsibility: each class does ONE thing
//   O — Open/Closed: open for extension, closed for modification
//   L — Liskov Substitution: subtypes can replace their base types
//   I — Interface Segregation: many small interfaces > one fat interface
//   D — Dependency Inversion: depend on abstractions, not concrete classes
//
// You don't need to memorize all of these right now. We'll see them
// in action throughout this project. For now, just notice how each
// class has a clear, focused purpose.
// =============================================================================

#pragma once

#include "core/thread_safe_hash_map.hpp"

#include <chrono> // For time-related types (steady_clock, duration)
#include <optional>
#include <string>
#include <vector>

namespace mini_redis {

// =============================================================================
// StoreEntry — What we actually store in the map
// =============================================================================
// WHAT IS A STRUCT?
// A struct is identical to a class, except members are PUBLIC by default
// (class members are PRIVATE by default). By convention:
//   - struct = plain data container (no complex behavior)
//   - class = has behavior, invariants, encapsulation
//
// StoreEntry is just data — the value string and an optional expiration
// time — so a struct is appropriate.
// =============================================================================
struct StoreEntry {
  // The actual value stored
  std::string value;

  // When this entry expires. std::nullopt means "never expires."
  //
  // WHY steady_clock AND NOT system_clock?
  // system_clock = wall clock (can jump forward/backward if the user
  //                changes the time, or NTP adjusts it)
  // steady_clock = monotonic clock (ALWAYS goes forward, never jumps)
  //
  // For measuring durations (like TTL), steady_clock is correct.
  // For displaying dates to users, system_clock is correct.
  std::optional<std::chrono::steady_clock::time_point> expires_at;
};

// =============================================================================
// KeyValueStore — The main storage interface
// =============================================================================
class KeyValueStore {
public:
  // ---- get() — Retrieve a value by key ----
  // Returns std::nullopt if:
  //   - Key doesn't exist, OR
  //   - Key exists but has expired (lazy deletion — we check on access)
  std::optional<std::string> get(const std::string &key);

  // ---- set() — Store a key-value pair ----
  // ttl_seconds: how many seconds until this key expires
  //   - 0 means "never expire"
  //   - Any positive value sets an expiration time
  void set(const std::string &key, const std::string &value,
           int ttl_seconds = 0);

  // ---- remove() — Delete a key ----
  // Returns true if the key existed and was removed
  bool remove(const std::string &key);

  // ---- keys() — List all non-expired keys ----
  std::vector<std::string> keys() const;

  // ---- cleanup_expired() — Remove all expired entries ----
  // Called periodically by the ExpiryManager background thread.
  // Returns the number of entries removed.
  std::size_t cleanup_expired();

private:
  // ---- Helper: check if an entry has expired ----
  // "static" here means the function doesn't need an object to call it
  // AND it doesn't access any member variables.
  static bool is_expired(const StoreEntry &entry);

  // ---- Helper: calculate expiration time point ----
  static std::optional<std::chrono::steady_clock::time_point>
  calculate_expiry(int ttl_seconds);

  // The underlying thread-safe map
  // Key = std::string (the key name)
  // Value = StoreEntry (value + expiration)
  ThreadSafeHashMap<std::string, StoreEntry> store_;
};

} // namespace mini_redis
