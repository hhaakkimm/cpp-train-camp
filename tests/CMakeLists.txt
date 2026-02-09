# =============================================================================
# tests/CMakeLists.txt — Build configuration for unit tests
# =============================================================================
#
# WHAT ARE UNIT TESTS?
# In competitive programming, you test by submitting to the judge.
# In industry, you write automated tests that run in seconds and catch bugs
# BEFORE code goes to production. Google Test is the most popular C++ testing
# framework — it's what Google uses internally for their C++ codebases.
# =============================================================================

# --- FetchContent: Download Google Test automatically ---
# Instead of asking the developer to install Google Test manually,
# FetchContent downloads it during the CMake configuration step.
# This makes the project easy to build on any machine.
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# --- Enable CTest (CMake's test runner) ---
# This lets us run all tests with "ctest" command
enable_testing()

# --- List the source files we need for tests ---
# Tests need the actual source files too (except main.cpp, since Google Test
# provides its own main function via gtest_main)
set(TESTABLE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/thread_safe_hash_map.cpp
    ${CMAKE_SOURCE_DIR}/src/core/key_value_store.cpp
    ${CMAKE_SOURCE_DIR}/src/core/expiry_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/http/http_request.cpp
    ${CMAKE_SOURCE_DIR}/src/http/http_response.cpp
    ${CMAKE_SOURCE_DIR}/src/util/logger.cpp
)

# --- Test: Key Value Store ---
add_executable(test_key_value_store
    test_key_value_store.cpp
    ${TESTABLE_SOURCES}
)
target_include_directories(test_key_value_store
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_key_value_store
    PRIVATE GTest::gtest_main Threads::Threads
)
# add_test() registers the test with CTest so "ctest" can find and run it
add_test(NAME KeyValueStoreTests COMMAND test_key_value_store)

# --- Test: HTTP Request Parser ---
add_executable(test_http_request
    test_http_request.cpp
    ${TESTABLE_SOURCES}
)
target_include_directories(test_http_request
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_http_request
    PRIVATE GTest::gtest_main Threads::Threads
)
add_test(NAME HttpRequestTests COMMAND test_http_request)
