// =============================================================================
// test_key_value_store.cpp — Unit Tests for the Key-Value Store
// =============================================================================
//
// WHAT ARE UNIT TESTS?
// A unit test verifies that ONE small piece of code works correctly in
// isolation.
//
// GOOGLE TEST (gtest) BASICS:
// - TEST(TestSuiteName, TestName) { ... } — defines a single test
// - EXPECT_EQ(a, b)     — checks that a == b (test continues if it fails)
// - ASSERT_EQ(a, b)     — checks that a == b (test STOPS if it fails)
// - EXPECT_TRUE(expr)   — checks that expr is true
// - EXPECT_FALSE(expr)  — checks that expr is false
// - EXPECT_NE(a, b)     — checks that a != b (NE = Not Equal)
//
// WHY UNIT TESTS?
// 1. Catch bugs early (before they reach production)
// 2. Serve as documentation (show how to USE the code)
// 3. Enable confident refactoring (if tests pass, the change is safe)
// 4. Required for professional C++ development (interviewers ask about testing)
//
// RUN TESTS WITH: cd build && ctest --output-on-failure
// =============================================================================

// Google Test framework header
#include <gtest/gtest.h>

// The code we're testing
#include "core/key_value_store.hpp"

// For sleep (testing TTL expiration)
#include <chrono>
#include <thread>

// =============================================================================
// TEST SUITE: KeyValueStoreTest
// =============================================================================
// All tests in a suite share a name prefix. This groups related tests together
// in the test output:
//   [==========] Running 6 tests from 1 test suite.
//   [ RUN      ] KeyValueStoreTest.SetAndGet
//   [       OK ] KeyValueStoreTest.SetAndGet (0 ms)
//   ...
// =============================================================================

// --- Test: basic set and get ---
TEST(KeyValueStoreTest, SetAndGet) {
  // ARRANGE: create a store and set a value
  mini_redis::KeyValueStore store;
  store.set("greeting", "Hello, World!");

  // ACT: retrieve the value
  const auto result = store.get("greeting");

  // ASSERT: verify the result
  //
  // WHY THIS PATTERN? (Arrange-Act-Assert / AAA)
  // It's the standard structure for unit tests:
  //   1. ARRANGE: set up the test data and preconditions
  //   2. ACT: call the function being tested
  //   3. ASSERT: verify the output/side-effects
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "Hello, World!");
}

// --- Test: getting a key that doesn't exist ---
TEST(KeyValueStoreTest, GetNonExistentKey) {
  mini_redis::KeyValueStore store;

  const auto result = store.get("nonexistent");

  // Should return std::nullopt (empty optional)
  EXPECT_FALSE(result.has_value());
}

// --- Test: overwriting an existing key ---
TEST(KeyValueStoreTest, OverwriteExistingKey) {
  mini_redis::KeyValueStore store;
  store.set("key", "old_value");
  store.set("key", "new_value"); // Overwrite

  const auto result = store.get("key");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "new_value");
}

// --- Test: deleting a key ---
TEST(KeyValueStoreTest, DeleteKey) {
  mini_redis::KeyValueStore store;
  store.set("to_delete", "some_value");

  // Delete should return true (key existed)
  EXPECT_TRUE(store.remove("to_delete"));

  // Now get should return nullopt
  EXPECT_FALSE(store.get("to_delete").has_value());

  // Deleting again should return false (key no longer exists)
  EXPECT_FALSE(store.remove("to_delete"));
}

// --- Test: listing all keys ---
TEST(KeyValueStoreTest, ListKeys) {
  mini_redis::KeyValueStore store;
  store.set("alpha", "1");
  store.set("beta", "2");
  store.set("gamma", "3");

  const auto all_keys = store.keys();

  // We should have exactly 3 keys
  EXPECT_EQ(all_keys.size(), 3u);

  // Check that each key is present (order is NOT guaranteed in hash maps)
  // std::find checks if an element exists in a container
  EXPECT_NE(std::find(all_keys.begin(), all_keys.end(), "alpha"),
            all_keys.end());
  EXPECT_NE(std::find(all_keys.begin(), all_keys.end(), "beta"),
            all_keys.end());
  EXPECT_NE(std::find(all_keys.begin(), all_keys.end(), "gamma"),
            all_keys.end());
}

// --- Test: TTL expiration ---
TEST(KeyValueStoreTest, TTLExpiration) {
  mini_redis::KeyValueStore store;

  // Set a key with 1-second TTL
  store.set("temp_key", "temp_value", 1);

  // Should exist immediately
  ASSERT_TRUE(store.get("temp_key").has_value());
  EXPECT_EQ(store.get("temp_key").value(), "temp_value");

  // Wait for the key to expire
  // std::this_thread::sleep_for pauses the current thread
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Should be gone now (lazy deletion triggers on get())
  EXPECT_FALSE(store.get("temp_key").has_value());
}

// --- Test: cleanup_expired removes expired entries ---
TEST(KeyValueStoreTest, CleanupExpired) {
  mini_redis::KeyValueStore store;

  // Set a permanent key and a temporary key
  store.set("permanent", "stays forever");
  store.set("temporary", "goes away", 1); // 1-second TTL

  // Wait for the temporary key to expire
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Run cleanup
  const std::size_t removed = store.cleanup_expired();

  // Should have removed exactly 1 entry (the expired one)
  EXPECT_EQ(removed, 1u);

  // Permanent key should still exist
  ASSERT_TRUE(store.get("permanent").has_value());
  EXPECT_EQ(store.get("permanent").value(), "stays forever");
}
