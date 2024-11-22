#ifndef COMPILER_DEFS_H
#define COMPILER_DEFS_H

#if defined(_MSC_VER)
    // Code for MSVC on Windows
#elif defined(__clang__)
    // Code for Clang
#elif defined(__GNUC__)
    // Code for GCC (Linux or macOS)
#else
    #error "Unknown compiler"
#endif

#endif