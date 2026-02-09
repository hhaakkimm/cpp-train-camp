# =============================================================================
# ROOT CMakeLists.txt — The "Project Manager" of Your C++ Build
# =============================================================================
#
# WHAT IS CMAKE?
# In competitive programming, you just run "g++ main.cpp -o main". That works
# for 1 file. But industrial projects have 20-50+ source files, external
# libraries, tests, and multiple build configurations (Debug vs Release).
#
# CMake is a BUILD SYSTEM GENERATOR — it reads this file and generates the
# actual build instructions (Makefiles on Linux/Mac, .sln on Windows).
#
# HOW TO BUILD THIS PROJECT:
#   mkdir build && cd build    ← keep build files separate from source code
#   cmake ..                   ← generate Makefiles from CMakeLists.txt
#   make -j$(nproc)            ← compile everything using all CPU cores
# =============================================================================

# --- Minimum CMake version required ---
# We need 3.14+ for the "FetchContent" feature (downloads Google Test for us)
cmake_minimum_required(VERSION 3.14)

# --- Project declaration ---
# project() tells CMake:
#   - The project name (used in IDE titles, package names, etc.)
#   - VERSION: semantic versioning (major.minor.patch)
#   - LANGUAGES: which compilers to look for (CXX = C++)
project(mini_redis
    VERSION 1.0.0
    LANGUAGES CXX
)

# --- Set the C++ standard ---
# CMAKE_CXX_STANDARD: which C++ version to use (17 = C++17)
# C++17 gives us: std::optional, std::string_view, structured bindings,
#                 std::shared_mutex, if-init-statements, and more
#
# CMAKE_CXX_STANDARD_REQUIRED: if the compiler can't do C++17, STOP with error
# CMAKE_CXX_EXTENSIONS: OFF means don't use compiler-specific extensions (GNU)
#                        This makes code portable across compilers
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Compiler warnings ---
# In competitive programming, you ignore warnings. In industry, warnings catch bugs.
#
# -Wall      : enable most warnings (uninitialized vars, unused vars, etc.)
# -Wextra    : enable extra warnings beyond -Wall
# -Wpedantic : warn if you use non-standard C++ features
# -Werror    : treat ALL warnings as errors (won't compile until you fix them!)
#
# add_compile_options() applies these flags to EVERY target in this project
add_compile_options(-Wall -Wextra -Wpedantic)

# --- Export compile commands ---
# This creates a "compile_commands.json" file that IDEs (VS Code, CLion)
# use to provide code intelligence (autocomplete, go-to-definition, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Add subdirectories ---
# Each subdirectory has its own CMakeLists.txt with more build instructions.
# CMake processes them in order.
# "src" contains the main application code
# "tests" contains unit tests (Google Test)
add_subdirectory(src)
add_subdirectory(tests)
