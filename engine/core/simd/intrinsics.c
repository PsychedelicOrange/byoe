#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <stddef.h>

#if defined(_MSC_VER)    // MSVC Compiler

    // MSVC specific prefetch (similar to __builtin_prefetch in GCC/Clang)
    #include <intrin.h>

void prefetch(const void* ptr)
{
    _mm_prefetch((const char*) ptr, _MM_HINT_T0);    // T0: Most temporal locality
}

// MSVC equivalent to __builtin_expect
int likely(int expr)
{
    __assume(expr);    // MSVC doesn't have built-in likely/unlikely, use __assume
    return expr;
}

int unlikely(int expr)
{
    __assume(!expr);    // Assumes the expression is false
    return expr;
}

#elif defined(__clang__) || defined(__GNUC__)    // GCC/Clang compilers

// GCC/Clang: __builtin_prefetch
void prefetch(const void* ptr)
{
    (void) ptr;
    __builtin_prefetch(ptr, 0, 3);    // 0: Read, 3: High temporal locality
}

// GCC/Clang: __builtin_expect for likely/unlikely branch predictions
int likely(int expr)
{
    return __builtin_expect(!!(expr), 1);    // Optimizer hint for likely conditions
}

int unlikely(int expr)
{
    return __builtin_expect(!!(expr), 0);    // Optimizer hint for unlikely conditions
}

#else
    #error "Unknown compiler! Please provide a prefetch and likely/unlikely mechanism."
#endif

// Function to perform prefetch for an array
void prefetch_array(const void* array, size_t size)
{
    const char* ptr = (const char*) array;
    for (size_t i = 0; i < size; i += 64) {    // Assuming 64 bytes per cache line
        prefetch(ptr + i);
    }
}

#endif