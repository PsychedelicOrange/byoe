#include "test_hash_map.h"
#include "test_uuid.h"
#include "test_rng.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    //test_hash_map();
    test_uuid();
    test_rng();

    // Benchmarks
    benchmark_hash_map();

    return EXIT_SUCCESS;
}