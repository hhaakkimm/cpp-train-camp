# =============================================================================
# src/CMakeLists.txt — Build configuration for the main application
# =============================================================================
#
# This file tells CMake HOW to compile all our source files into one executable.
# =============================================================================

# --- Collect all source files ---
# We list every .cpp file that needs to be compiled.
# Header files (.hpp) are NOT listed here — they are included via #include.
# CMake only needs to know about files that produce "object code" (.o files).
set(MINI_REDIS_SOURCES
    main.cpp
    core/thread_safe_hash_map.cpp
    core/key_value_store.cpp
    core/expiry_manager.cpp
    network/socket.cpp
    network/tcp_server.cpp
    http/http_request.cpp
    http/http_response.cpp
    http/router.cpp
    api/kv_handler.cpp
    util/thread_pool.cpp
    util/logger.cpp
    app/application.cpp
)

# --- Create the executable target ---
# add_executable(target_name, source_files...)
# This tells CMake: "Compile these files and link them into an executable
# called 'mini_redis'"
add_executable(mini_redis ${MINI_REDIS_SOURCES})

# --- Set include directories ---
# target_include_directories() tells the compiler WHERE to look when it sees
# #include "something.hpp"
#
# PUBLIC  = this target AND anything that links to it can see these dirs
# PRIVATE = only this target can see these dirs
# 
# CMAKE_CURRENT_SOURCE_DIR = the directory where THIS CMakeLists.txt lives
# So we're saying: "When compiling mini_redis, also look for headers in src/"
# This lets us write: #include "core/key_value_store.hpp" instead of
#                      #include "../core/key_value_store.hpp"
target_include_directories(mini_redis
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# --- Link system libraries ---
# On Linux/Mac, threading (std::thread, std::mutex) requires linking the
# pthread library. find_package(Threads) finds it portably.
find_package(Threads REQUIRED)
target_link_libraries(mini_redis PRIVATE Threads::Threads)
