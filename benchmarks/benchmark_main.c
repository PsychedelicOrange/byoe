#include "benchmark.h"
#include "benchmark_hash_map.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Benchmarks
    benchmark_hash_map();

    return EXIT_SUCCESS;
}
