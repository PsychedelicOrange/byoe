#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <stddef.h>

// Compiler-specific branch prediction and prefetch

#if defined(__clang__) || defined(__GNUC__)

// GCC/Clang: available on all platforms (x86, ARM, etc.)
void prefetch(const void* ptr)
{
    __builtin_prefetch(ptr, 0, 3);    // 0: read, 3: high temporal locality
}

int likely(int expr)
{
    return __builtin_expect(!!(expr), 1);
}

int unlikely(int expr)
{
    return __builtin_expect(!!(expr), 0);
}

#elif defined(_MSC_VER)

    #if defined(_M_IX86) || defined(_M_X64)
        #include <intrin.h>

void prefetch(const void* ptr)
{
    _mm_prefetch((const char*) ptr, _MM_HINT_T0);
}
    #elif defined(_M_ARM64)
// No intrin.h or prefetch on ARM64 safely, fallback to no-op
void prefetch(const void* ptr)
{
    (void) ptr;
}
    #else
        #error "Unsupported MSVC architecture"
    #endif

// MSVC doesn't have __builtin_expect; __assume is a hint
int likely(int expr)
{
    __assume(expr);
    return expr;
}

int unlikely(int expr)
{
    __assume(!expr);
    return expr;
}

#else
    #error "Unsupported compiler. Please define prefetch, likely, and unlikely for your platform."
#endif

// Array-wide prefetch utility (platform-agnostic)

void prefetch_array(const void* array, size_t size)
{
    const char* ptr = (const char*) array;
    for (size_t i = 0; i < size; i += 64) {    // 64-byte assumed cache line
        prefetch(ptr + i);
    }
}

#endif    // INTRINSICS_H
