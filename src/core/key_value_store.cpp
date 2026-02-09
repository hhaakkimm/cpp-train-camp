// =============================================================================
// key_value_store.cpp — High-Level Key-Value Store (IMPLEMENTATION)
// =============================================================================

#include "core/key_value_store.hpp"
#include "util/logger.hpp"

namespace mini_redis {

// =============================================================================
// get() — Retrieve a value, checking for expiration
// =============================================================================
// This implements LAZY DELETION:
// Instead of removing expired keys immediately when they expire (which would
// require a timer for EVERY key), we check if the key is expired when
// someone tries to read it. If expired, we delete it and return "not found."
//
// This is how real Redis works too! It combines lazy deletion (on access)
// with periodic cleanup (background thread) for keys nobody accesses.
// =============================================================================
std::optional<std::string> KeyValueStore::get(const std::string &key) {
  // Retrieve the entry from the thread-safe map
  const auto entry = store_.get(key);

  // If key doesn't exist, return nullopt
  if (!entry.has_value()) {
    return std::nullopt;
  }

  // If key exists but is expired, remove it and return nullopt
  if (is_expired(entry.value())) {
    store_.remove(key);
    Logger::info("Key '" + key + "' expired (lazy deletion)");
    return std::nullopt;
  }

  // Key exists and is not expired — return the value
  return entry.value().value;
}

// =============================================================================
// set() — Store a key-value pair with optional TTL
// =============================================================================
void KeyValueStore::set(const std::string &key, const std::string &value,
                        int ttl_seconds) {
  // Create the StoreEntry using aggregate initialization (C++11)
  // The {curly braces} syntax initializes each field in order:
  //   .value = the string value
  //   .expires_at = calculated expiration time (or nullopt if ttl_seconds == 0)
  StoreEntry entry{value, calculate_expiry(ttl_seconds)};

  // Store it in the thread-safe map
  store_.set(key, entry);

  // Log what we did
  if (ttl_seconds > 0) {
    Logger::info("SET '" + key + "' (TTL: " + std::to_string(ttl_seconds) +
                 "s)");
  } else {
    Logger::info("SET '" + key + "' (no expiry)");
  }
}

// =============================================================================
// remove() — Delete a key-value pair
// =============================================================================
bool KeyValueStore::remove(const std::string &key) {
  const bool removed = store_.remove(key);

  if (removed) {
    Logger::info("DEL '" + key + "' — removed");
  } else {
    Logger::info("DEL '" + key + "' — key not found");
  }

  return removed;
}

// =============================================================================
// keys() — List all non-expired keys
// =============================================================================
std::vector<std::string> KeyValueStore::keys() const {
  std::vector<std::string> result;

  // Use for_each to iterate under the read lock
  // The lambda captures 'result' by REFERENCE (&result) so it can
  // add keys to our local vector from inside the callback.
  store_.for_each([&result](const std::string &key, const StoreEntry &entry) {
    // Only include non-expired keys
    if (!is_expired(entry)) {
      result.push_back(key);
    }
  });

  return result;
}

// =============================================================================
// cleanup_expired() — Bulk remove all expired entries
// =============================================================================
std::size_t KeyValueStore::cleanup_expired() {
  // remove_if takes a predicate (a function that returns true/false).
  // For each entry where is_expired returns true, remove it.
  const std::size_t count = store_.remove_if(
      [](const std::string & /*key*/, const StoreEntry &entry) {
        //                ^^^^^^^^^
        // The /*key*/ notation means "I'm not using this parameter."
        // Commenting out the name prevents "unused parameter" warnings.
        return is_expired(entry);
      });

  if (count > 0) {
    Logger::info("Cleanup: removed " + std::to_string(count) +
                 " expired entries");
  }

  return count;
}

// =============================================================================
// is_expired() — Check if a StoreEntry has passed its expiration time
// =============================================================================
bool KeyValueStore::is_expired(const StoreEntry &entry) {
  // If no expiration time is set, the entry never expires
  if (!entry.expires_at.has_value()) {
    return false;
  }

  // Compare the expiration time with the current time
  // steady_clock::now() returns the current monotonic timestamp
  return std::chrono::steady_clock::now() >= entry.expires_at.value();
}

// =============================================================================
// calculate_expiry() — Convert TTL seconds to an absolute time point
// =============================================================================
std::optional<std::chrono::steady_clock::time_point>
KeyValueStore::calculate_expiry(int ttl_seconds) {
  // TTL of 0 means "no expiration"
  if (ttl_seconds <= 0) {
    return std::nullopt;
  }

  // now() + duration = future time point when the key should expire
  // std::chrono::seconds(n) creates a duration of n seconds
  return std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds);
}

} // namespace mini_redis
