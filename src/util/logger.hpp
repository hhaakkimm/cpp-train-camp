// =============================================================================
// logger.hpp — Thread-Safe Logging Utility (HEADER FILE)
// =============================================================================
//
// WHY A LOGGER?
// In competitive programming, you use "cout << x" to debug. In production,
// you need structured logging with:
//   - Timestamps (when did it happen?)
//   - Log levels (INFO vs ERROR — so you can filter)
//   - Thread safety (multiple threads printing at once = garbled text)
//
// WHAT IS A HEADER FILE (.hpp)?
// A header file is like a "menu" — it tells OTHER files what functions and
// classes are AVAILABLE, without showing the implementation details.
// The actual code lives in the matching .cpp file.
//
// Think of it like a restaurant menu (header) vs the kitchen (source file):
//   - Menu says "we have pizza" (declaration)
//   - Kitchen actually makes the pizza (definition)
//
// WHY SEPARATE .hpp AND .cpp?
// 1. Compile speed: changing the .cpp doesn't recompile files that #include
//    the .hpp (only the .cpp gets recompiled)
// 2. Encapsulation: users of your class only see the interface, not internals
// 3. Avoid duplicate symbol errors when multiple files include the same header
// =============================================================================

// --- Include guard ---
// #pragma once tells the compiler: "Only include this file ONE TIME, even if
// multiple .cpp files #include it."
//
// Without this, if file A includes this header and file B also includes it,
// and file C includes both A and B, you'd get "duplicate definition" errors.
//
// The older way is #ifndef/#define/#endif guards, but #pragma once is simpler
// and supported by every modern compiler (GCC, Clang, MSVC).
#pragma once

// --- Standard library includes ---
// <string> provides std::string — a safe, dynamically-sized string class.
// Unlike C-style char arrays, std::string manages its own memory automatically.
#include <string>

// <mutex> provides std::mutex — a "mutual exclusion" lock.
// When one thread locks a mutex, all other threads that try to lock it
// will WAIT until the first thread unlocks it. This prevents data races.
#include <mutex>

// =============================================================================
// WHAT IS A NAMESPACE?
// A namespace is like a "last name" for your code. Just like there can be
// two people named "John" (John Smith vs John Doe), there can be two classes
// named "Logger" in different libraries.
//
// By putting our code inside "namespace mini_redis", we avoid name collisions:
//   - mini_redis::Logger (ours)
//   - some_other_lib::Logger (someone else's)
//
// In competitive programming, you write "using namespace std;" to avoid
// typing "std::" everywhere. In industry, this is FORBIDDEN because:
// 1. If two libraries both define "sort()", which one gets called? Ambiguity!
// 2. The std namespace has THOUSANDS of names — any of them could collide
//    with your own variable/function names without you knowing.
// =============================================================================
namespace mini_redis {

// =============================================================================
// WHAT IS AN ENUM CLASS?
// An "enum" (enumeration) gives names to a set of integer constants.
// "enum class" (scoped enum, C++11) is the SAFE version:
//
// Old-style enum:  enum Color { RED, GREEN };    // RED = 0, pollutes namespace
// Modern enum:     enum class Color { RED, GREEN }; // Must say Color::RED
//
// enum class advantages:
// 1. Won't accidentally convert to int (type safety)
// 2. Names don't leak into the surrounding scope
// 3. Can specify underlying type (we use "int" here, which is default)
// =============================================================================
enum class LogLevel {
    INFO,     // Normal operation messages ("Server started", "Key set")
    WARNING,  // Something unusual but not fatal ("Key not found")
    ERROR     // Something broke ("Failed to bind socket")
};

// =============================================================================
// THE Logger CLASS
// =============================================================================
// This is a UTILITY class with only static methods. You don't create Logger
// objects — you call Logger::info("message") directly.
//
// WHAT IS "static"?
// A static member belongs to the CLASS ITSELF, not to any particular object.
//   - Non-static: each Logger object would have its own mutex (wasteful)
//   - Static: there's ONE mutex shared by all, which is what we want since
//     there's only one stdout to protect
// =============================================================================
class Logger {
public:
    // ---- Public static methods ----
    // These are the functions other code will call.
    // "static" means you call them as Logger::info() not logger_object.info()

    // Log an informational message
    // const std::string& : pass by const reference (explained in .cpp)
    static void info(const std::string& message);

    // Log a warning message
    static void warning(const std::string& message);

    // Log an error message
    static void error(const std::string& message);

private:
    // ---- Private implementation ----
    // "private" means ONLY code inside this class can call these.
    // This is ENCAPSULATION — one of the pillars of OOP.
    // We hide the implementation detail that there's a "log" function
    // that all the public methods delegate to.

    // The actual logging function — writes to stdout with formatting
    static void log(LogLevel level, const std::string& message);

    // Converts LogLevel enum to its string name for display
    static std::string level_to_string(LogLevel level);

    // Gets current timestamp as a formatted string
    static std::string current_timestamp();

    // The mutex that prevents garbled output from concurrent threads.
    // "static inline" allows us to define (not just declare) the variable
    // right here in the header. Without "inline", we'd need a separate
    // definition in the .cpp file. (C++17 feature)
    static inline std::mutex log_mutex_;

    // NAMING CONVENTION: trailing underscore (log_mutex_) indicates a
    // private member variable. This is the Google C++ Style Guide convention.
    // It makes it instantly clear when reading code what is a local variable
    // vs a member variable.
};

} // namespace mini_redis
