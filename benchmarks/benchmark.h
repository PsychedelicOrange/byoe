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

#include <engine/core/common.h>


#define BENCHMARK_START(test_name) \
    printf(COLOR_PINK "[Benchmark] Starting: [%s]\n" COLOR_RESET, test_name); \
    uint64_t __start_time = benchmark_get_time();

#define BENCHMARK_END(test_name) \
    uint64_t __end_time = benchmark_get_time(); \
    double __duration = (double)(__end_time - __start_time) / benchmark_get_frequency(); \
    printf(COLOR_PINK "[Benchmark] Finished: [%s] in: %4.4f ms\n" COLOR_RESET, test_name, __duration * 1000.0f);

#define BENCHMARK_TIME() \
    __duration * 1000.0f // This retrieves the time spent from BENCHMARK_END

// printf(COLOR_YELLOW "[Benchmark] Running iteration %zu for benchmark: %s\n" COLOR_RESET, i + 1, test_name); \

// Macro for running a benchmark multiple times and calculating the average time
#define BENCHMARK_RUN_MULTIPLE(iterations, result, test_name, benchmark_code) \
    double __total_time = 0; \
    for (size_t i = 0; i < (iterations); ++i) { \
        uint64_t __start_time = benchmark_get_time(); \
        benchmark_code(); \
        uint64_t __end_time = benchmark_get_time(); \
        double __duration = (double)(__end_time - __start_time) / benchmark_get_frequency(); \
        __total_time += __duration * 1000.0f ; \
    } \
    result = __total_time / (iterations); \
    printf(COLOR_GREEN "[Benchmark] Average: [%s] for [%zu] runs: %4.4f ms\n" COLOR_RESET, \
            test_name, iterations, result);

// Macro for printing the results of multiple benchmarks
#define BENCHMARK_PRINT_RESULTS(test_name, average_time, iterations) \
    printf(COLOR_GREEN "[Benchmark] Results: [%s] for Average time over %zu iterations: %4.4f ms\n" COLOR_RESET, \
             test_name, iterations, average_time);

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
