#ifndef PLATFORM_DEFS_H
#define PLATFORM_DEFS_H

#if defined(_MSC_VER)
    #include <intrin.h>
#elif defined(__clang__) || defined(__GNUC__)
    #if defined(__APPLE__)
        #include <sys/types.h>
    #else
        #include <x86intrin.h>
    #endif
#else
    #error "Unknown compiler"
#endif

// Enum to represent different instruction sets
typedef enum SIMD {
    SIMD_NONE = 0,
    SIMD_SSE = 1 << 0,
    SIMD_SSE2 = 1 << 1,
    SIMD_SSE3 = 1 << 2,
    SIMD_SSSE3 = 1 << 3,
    SIMD_SSE4_1 = 1 << 4,
    SIMD_SSE4_2 = 1 << 5,
    SIMD_AVX = 1 << 6,
    SIMD_AVX2 = 1 << 7,
    SIMD_AVX512 = 1 << 8,
    SIMD_NEON = 1 << 9,   // ARM NEON support
    SIMD_ASIMD = 1 << 10  // ARMv8 ASIMD
} SIMD;

// Function to detect supported instruction sets
static inline int cpu_detect_instruction_set() {
    int supported = SIMD_NONE;

#if defined(_MSC_VER)
    // Windows - MSVC intrinsics
    int info[4];
    __cpuid(info, 0);  // Query for max CPUID
    if (info[0] >= 1) {
        __cpuid(info, 1);
        if (info[3] & (1 << 25)) supported |= SIMD_SSE;
        if (info[3] & (1 << 26)) supported |= SIMD_SSE2;
        if (info[2] & (1 << 0))  supported |= SIMD_SSE3;
        if (info[2] & (1 << 9))  supported |= SIMD_SSSE3;
        if (info[2] & (1 << 19)) supported |= SIMD_SSE4_1;
        if (info[2] & (1 << 20)) supported |= SIMD_SSE4_2;
        if (info[2] & (1 << 28)) supported |= SIMD_AVX;
    }
    __cpuid(info, 7);  // Extended features
    if (info[1] & (1 << 5))  supported |= SIMD_AVX2;
    if (info[1] & (1 << 16)) supported |= SIMD_AVX512;

#elif defined(__clang__) || defined(__GNUC__)
    // Linux/macOS/other platforms - GCC/Clang intrinsics
    #if defined(__x86_64__) || defined(__i386__)
        #if defined(__SSE__)
            supported |= SIMD_SSE;
        #endif
        #if defined(__SSE2__)
            supported |= SIMD_SSE2;
        #endif
        #if defined(__SSE3__)
            supported |= SIMD_SSE3;
        #endif
        #if defined(__SSSE3__)
            supported |= SIMD_SSSE3;
        #endif
        #if defined(__SSE4_1__)
            supported |= SIMD_SSE4_1;
        #endif
        #if defined(__SSE4_2__)
            supported |= SIMD_SSE4_2;
        #endif
        #if defined(__AVX__)
            supported |= SIMD_AVX;
        #endif
        #if defined(__AVX2__)
            supported |= SIMD_AVX2;
        #endif
        #if defined(__AVX512F__)
            supported |= SIMD_AVX512;
        #endif
    #elif defined(__aarch64__) || defined(__ARM_NEON__)
        // ARM NEON (common in mobile/Apple Silicon)
        supported |= SIMD_NEON;
        #if defined(__ARM_NEON__)
            supported |= SIMD_ASIMD;
        #endif
    #endif
#endif

    return supported;
}

#endif // PLATFORM_DEFS_H
