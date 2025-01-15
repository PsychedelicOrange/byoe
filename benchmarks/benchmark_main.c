#include "benchmark.h"
#include "benchmark_hash_map.h"

#include <engine/core/simd/platform_caps.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Print cpu/os info
    // cpu_caps_print_info();
    // os_caps_print_info();

    // Benchmarks
    benchmark_hash_map();

    return EXIT_SUCCESS;
}
