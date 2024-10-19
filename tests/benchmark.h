#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#define BENCHMARK_START() \
    uint64_t __start_time = benchmark_get_time();
    //printf("Starting benchmark: %s...\n", test_name);

#define BENCHMARK_END() \
    uint64_t __end_time = benchmark_get_time(); \
    double __duration = (double)(__end_time - __start_time) / benchmark_get_frequency();
    //printf("Benchmark finished: %s in %4.4f ms\n", test_name, __duration * 1000.0f);

#define BENCHMARK_TIME() \
    __duration * 1000.0f // This retrieves the time spent from BENCHMARK_END

#ifdef _WIN32
// Windows high-resolution time functions
static inline uint64_t benchmark_get_time() {
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return (uint64_t)time.QuadPart;
}

static inline uint64_t benchmark_get_frequency() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (uint64_t)freq.QuadPart;
}
#else
// Unix-based systems use gettimeofday for microsecond precision
static inline uint64_t benchmark_get_time(void) {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (uint64_t)(time.tv_sec * 1000000ULL + time.tv_usec);
}

static inline uint64_t benchmark_get_frequency(void) {
    return 1000000ULL;  // Microseconds to seconds
}
#endif

#endif  // BENCHMARK_H
