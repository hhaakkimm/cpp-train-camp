# 🔴 Mini Redis — Learn Industrial C++ by Building

An in-memory key-value store with HTTP REST API, built from scratch in **modern C++17**.

Every file is heavily commented to teach **industrial C++ concepts** to competitive programmers making the transition to real-world software engineering.

---

## ✨ Features

- **Key-Value Store** — GET, PUT, DELETE, KEYS operations
- **TTL Expiration** — keys auto-expire with background cleanup
- **Thread Safety** — concurrent access via `std::shared_mutex`
- **HTTP REST API** — full HTTP/1.1 parser over raw TCP sockets
- **Thread Pool** — fixed-size pool for handling connections
- **RAII Everywhere** — sockets, locks, threads all cleanup automatically
- **Graceful Shutdown** — Ctrl+C triggers clean teardown

---

## 🏗️ Quick Start

```bash
# Clone and build
git clone git@github.com:hhaakkimm/cpp-train-camp.git && cd cpp-train-camp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(sysctl -n hw.ncpu)    # macOS
# make -j$(nproc)              # Linux

# Run the server
./src/mini_redis

# In another terminal — try the API:
curl -X PUT http://localhost:8080/kv/hello -d "world"
curl http://localhost:8080/kv/hello          # → world
curl -X PUT -H "X-TTL: 10" http://localhost:8080/kv/temp -d "gone in 10s"
curl http://localhost:8080/kv                # → list all keys
curl -X DELETE http://localhost:8080/kv/hello

# Run tests
./tests/test_key_value_store    # 7 tests
./tests/test_http_request       # 6 tests
```

---

## 📖 How to Read This Project

> **This project is designed to be read like a textbook.** Each file teaches specific industrial C++ concepts, building on the previous ones. Follow the reading order below for the best learning experience.

### 🗺️ Recommended Reading Order

Read the files in this order — each one introduces new concepts that later files depend on:

#### Step 1: Build System — *"How industrial projects are built"*

| File | You'll Learn |
|---|---|
| [`CMakeLists.txt`](CMakeLists.txt) | CMake basics, compiler warnings, C++17 setup |
| [`src/CMakeLists.txt`](src/CMakeLists.txt) | Targets, source file lists, include directories |
| [`tests/CMakeLists.txt`](tests/CMakeLists.txt) | FetchContent, Google Test, CTest |

#### Step 2: Utilities — *"Tools every C++ project needs"*

| File | You'll Learn |
|---|---|
| [`src/util/logger.hpp`](src/util/logger.hpp) | `#pragma once`, namespaces, `enum class`, `static` members |
| [`src/util/logger.cpp`](src/util/logger.cpp) | `std::mutex`, `std::lock_guard`, RAII for thread safety |
| [`src/util/thread_pool.hpp`](src/util/thread_pool.hpp) | `std::function`, lambdas, `explicit`, Rule of Five, `= delete` |
| [`src/util/thread_pool.cpp`](src/util/thread_pool.cpp) | `std::move`, `unique_lock` vs `lock_guard`, `condition_variable` |

#### Step 3: Core Storage — *"The heart of the database"*

| File | You'll Learn |
|---|---|
| [`src/core/thread_safe_hash_map.hpp`](src/core/thread_safe_hash_map.hpp) | Templates, `std::optional`, `std::shared_mutex`, `mutable`, structured bindings |
| [`src/core/key_value_store.hpp`](src/core/key_value_store.hpp) | SOLID principles, `struct` vs `class`, `std::chrono`, lazy deletion |
| [`src/core/key_value_store.cpp`](src/core/key_value_store.cpp) | Aggregate initialization, time point comparisons |
| [`src/core/expiry_manager.hpp`](src/core/expiry_manager.hpp) | Background threads, dependency injection, references vs pointers |
| [`src/core/expiry_manager.cpp`](src/core/expiry_manager.cpp) | `std::atomic`, interruptible sleep with `condition_variable::wait_for` |

#### Step 4: Networking — *"Talking to the outside world"*

| File | You'll Learn |
|---|---|
| [`src/network/socket.hpp`](src/network/socket.hpp) | RAII for OS resources, move semantics (`&&`), `noexcept`, factory pattern |
| [`src/network/socket.cpp`](src/network/socket.cpp) | `std::exchange`, POSIX sockets, `htons`, `static_cast` vs C-cast, `reinterpret_cast` |
| [`src/network/tcp_server.hpp`](src/network/tcp_server.hpp) | Type aliases, thread-per-request model |
| [`src/network/tcp_server.cpp`](src/network/tcp_server.cpp) | `std::shared_ptr`, `auto`, accept loop pattern, lambda captures |

#### Step 5: HTTP Layer — *"Understanding the web protocol"*

| File | You'll Learn |
|---|---|
| [`src/http/http_request.hpp`](src/http/http_request.hpp) | HTTP format, factory pattern, encapsulation, `string_view` |
| [`src/http/http_request.cpp`](src/http/http_request.cpp) | `std::istringstream`, anonymous namespaces, `istreambuf_iterator` |
| [`src/http/http_response.hpp`](src/http/http_response.hpp) | Builder design pattern, method chaining |
| [`src/http/http_response.cpp`](src/http/http_response.cpp) | HTTP serialization, `Content-Length`, `Connection: close` |
| [`src/http/router.hpp`](src/http/router.hpp) | REST routing, path parameters, `std::function` with captures |
| [`src/http/router.cpp`](src/http/router.cpp) | First-match routing, prefix matching, suffix extraction |

#### Step 6: Application — *"Wiring it all together"*

| File | You'll Learn |
|---|---|
| [`src/api/kv_handler.hpp`](src/api/kv_handler.hpp) | Layered architecture, separation of concerns |
| [`src/api/kv_handler.cpp`](src/api/kv_handler.cpp) | Lambda bridging, `std::stoi`, REST endpoint implementation |
| [`src/app/application.hpp`](src/app/application.hpp) | Composition vs inheritance, member initialization order |
| [`src/app/application.cpp`](src/app/application.cpp) | Component wiring, `std::atomic::exchange`, destruction order |
| [`src/main.cpp`](src/main.cpp) | Signal handling, `SIGINT`, stack vs heap allocation |

#### Step 7: Tests — *"Proving it works"*

| File | You'll Learn |
|---|---|
| [`tests/test_key_value_store.cpp`](tests/test_key_value_store.cpp) | Google Test, `EXPECT_EQ`/`ASSERT_TRUE`, AAA pattern |
| [`tests/test_http_request.cpp`](tests/test_http_request.cpp) | Testing parsers, edge cases, nullopt checks |

---

## 🏛️ Architecture

```
Client (curl)
    │
    │ HTTP over TCP
    ▼
┌──────────────────────────────────────┐
│              TcpServer               │
│         (accept loop + Socket)       │
│                  │                   │
│           ┌──────┴──────┐            │
│           │  ThreadPool │            │
│           └──────┬──────┘            │
│                  │                   │
│         ┌────────┴────────┐          │
│         │  HTTP Pipeline  │          │
│         │ Request→Router  │          │
│         │ →Handler→Response          │
│         └────────┬────────┘          │
│                  │                   │
│         ┌────────┴────────┐          │
│         │  KeyValueStore  │          │
│         │  (ThreadSafe    │          │
│         │   HashMap)      │          │
│         └────────┬────────┘          │
│                  │                   │
│         ┌────────┴────────┐          │
│         │ ExpiryManager   │          │
│         │ (background TTL)│          │
│         └─────────────────┘          │
└──────────────────────────────────────┘
```

---

## 📚 Concepts Index

Find any concept quickly:

| Concept | File(s) |
|---|---|
| RAII | `socket.hpp`, `logger.cpp`, `thread_pool.cpp` |
| Move Semantics | `socket.cpp`, `thread_pool.cpp` |
| Smart Pointers | `tcp_server.cpp` |
| Templates | `thread_safe_hash_map.hpp` |
| `std::optional` | `thread_safe_hash_map.hpp`, `key_value_store.cpp` |
| `std::shared_mutex` | `thread_safe_hash_map.hpp` |
| `std::condition_variable` | `thread_pool.cpp`, `expiry_manager.cpp` |
| `std::atomic` | `thread_pool.hpp`, `expiry_manager.hpp` |
| Builder Pattern | `http_response.hpp` |
| Factory Pattern | `socket.hpp`, `http_request.hpp` |
| SOLID Principles | `key_value_store.hpp` |
| Composition | `application.hpp` |
| Signal Handling | `main.cpp` |
| Anonymous Namespace | `http_request.cpp` |
| Rule of Five | `thread_pool.hpp`, `socket.hpp` |

---

## 🧪 Tests

```
13/13 tests passing ✅

KeyValueStoreTest:
  ✅ SetAndGet
  ✅ GetNonExistentKey
  ✅ OverwriteExistingKey
  ✅ DeleteKey
  ✅ ListKeys
  ✅ TTLExpiration
  ✅ CleanupExpired

HttpRequestTest:
  ✅ ParseGetRequest
  ✅ ParsePutRequestWithBody
  ✅ ParseDeleteRequest
  ✅ EmptyInputReturnsNullopt
  ✅ MalformedRequestLine
  ✅ HeaderLookupCaseInsensitive
```

---

## 📂 Project Structure

```
cpp-train-camp/
├── CMakeLists.txt              # Root build config
├── README.md                   # ← You are here
├── .gitignore
├── src/
│   ├── CMakeLists.txt          # Source build config
│   ├── main.cpp                # Entry point
│   ├── app/
│   │   ├── application.hpp     # Top-level orchestrator
│   │   └── application.cpp
│   ├── api/
│   │   ├── kv_handler.hpp      # REST endpoint handlers
│   │   └── kv_handler.cpp
│   ├── core/
│   │   ├── thread_safe_hash_map.hpp  # Concurrent map template
│   │   ├── thread_safe_hash_map.cpp
│   │   ├── key_value_store.hpp       # Business logic
│   │   ├── key_value_store.cpp
│   │   ├── expiry_manager.hpp        # Background TTL cleanup
│   │   └── expiry_manager.cpp
│   ├── http/
│   │   ├── http_request.hpp    # HTTP parser
│   │   ├── http_request.cpp
│   │   ├── http_response.hpp   # HTTP builder
│   │   ├── http_response.cpp
│   │   ├── router.hpp          # URL routing
│   │   └── router.cpp
│   ├── network/
│   │   ├── socket.hpp          # RAII socket wrapper
│   │   ├── socket.cpp
│   │   ├── tcp_server.hpp      # Connection manager
│   │   └── tcp_server.cpp
│   └── util/
│       ├── logger.hpp          # Thread-safe logging
│       ├── logger.cpp
│       ├── thread_pool.hpp     # Worker threads
│       └── thread_pool.cpp
└── tests/
    ├── CMakeLists.txt
    ├── test_key_value_store.cpp
    └── test_http_request.cpp
```

---

## 📄 License

Educational project — use freely for learning.
