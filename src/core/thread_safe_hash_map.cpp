// =============================================================================
// thread_safe_hash_map.cpp — Explicit Template Instantiation
// =============================================================================
//
// WHAT IS THIS FILE?
// Since template implementations live in the header, this .cpp file might
// seem unnecessary. However, it serves as a place for:
//
// 1. EXPLICIT TEMPLATE INSTANTIATION — forces the compiler to generate
//    code for specific type combinations HERE, which can speed up compilation
//    in large projects (other .cpp files don't need to re-generate the code).
//
// 2. Future non-template additions to the module.
//
// For our small project, the header-only approach works fine, but having
// this file demonstrates the industrial practice.
// =============================================================================

#include "core/thread_safe_hash_map.hpp"

// No additional implementation needed — all template code is in the header.
// The #include above ensures this translation unit compiles correctly.
