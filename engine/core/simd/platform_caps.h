#ifndef PLATFORM_DEFS_H
#define PLATFORM_DEFS_H

typedef enum SIMD
{
    SIMD_NONE   = 0,
    SIMD_SSE    = 1 << 0,
    SIMD_SSE2   = 1 << 1,
    SIMD_SSE3   = 1 << 2,
    SIMD_SSSE3  = 1 << 3,
    SIMD_SSE4_1 = 1 << 4,
    SIMD_SSE4_2 = 1 << 5,
    SIMD_AVX    = 1 << 6,
    SIMD_AVX2   = 1 << 7,
    SIMD_AVX512 = 1 << 8,
    SIMD_NEON   = 1 << 9,    // ARM NEON support
    SIMD_ASIMD  = 1 << 10    // ARMv8 ASIMD
} SIMD;

int cpu_detect_instruction_set(void);

void cpu_caps_print_info(void);

void os_caps_print_info(void);
#endif    // PLATFORM_DEFS_H
