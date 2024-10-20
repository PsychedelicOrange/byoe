#include <stdio.h>
#include <string.h>

#include "benchmark.h"

#include <engine/core/containers/hash_map.h>
#include <engine/core/logging/log.h>
#include <engine/core/uuid/uuid.h>

char* words[] = {
    "apple", "banana", "cherry", "date", "elderberry",
    "fig", "grape", "honeydew", "kiwi", "lemon",
    "mango", "nectarine", "orange", "papaya", "quince",
    "raspberry", "strawberry", "tangerine", "dragonfruit", "watermelon"
};

char* get_random_word(void) {
    int num_words = sizeof(words) / sizeof(words[0]);
    int random_index = rand() % num_words;  // Get a random index
    return words[random_index];
}

#define STEPS    1024 * 4
#define NUM_RUNS 1024 * 4

size_t total_found_count = 0; // To accumulate total found UUIDs over all runs

void avg_runs_benchmark(void)
{
    hash_map_t* map = hash_map_create(NUM_RUNS);

    // Array to store UUIDs for later retrieval
    random_uuid_t* uuids = malloc(NUM_RUNS * sizeof(random_uuid_t));

    // Generate UUIDs and store them in the array while setting values
    for (size_t i = 0; i < NUM_RUNS; i++) {
        uuid_generate(&uuids[i]);
        hash_map_set_key_value(map, uuids[i], get_random_word());
    }

    // Retrieve and verify each UUID
    size_t found_count = 0;
    for (size_t i = 0; i < NUM_RUNS; i++) {
        void* value = hash_map_get_value(map, uuids[i]);
        if (value != NULL) {
            found_count++;
        }
    }

    // Accumulate found counts
    total_found_count += found_count;

    free(uuids); // Clean up allocated memory

    // Free the map after each run
    hash_map_destroy(map);
}

void benchmark_hash_map(void)
{
    setvbuf(stdout, NULL, _IOFBF, 1024);  // Fully buffered with a 1KB buffer
    setbuf(stdout, NULL);

    hash_map_t* map = NULL;
    map = hash_map_create(STEPS);


    srand((unsigned int)time(NULL));  // Seed for random number generator

    //---------------------------------------
    {
       const char* benchmark_name = "Generate Random Words";
       BENCHMARK_START(benchmark_name);
       for (size_t i = 0; i < STEPS; i++)
       {
            get_random_word();
       }
       BENCHMARK_END(benchmark_name);
    }

    //---------------------------------------
    // create a bunch of UUIDs and add the English alphabet as the value

    {
       const char* benchmark_name = "Set Key(UUID) Value(Random Word) pair";
       BENCHMARK_START(benchmark_name);
       for (size_t i = 0; i < STEPS; i++)
       {
           random_uuid_t uuid;
           uuid_generate(&uuid);
           hash_map_set_key_value(map, uuid, get_random_word());
       }
       BENCHMARK_END(benchmark_name);

       hash_map_destroy(map);
    }

    //---------------------------------------
    // retrieve each UUID

    const char* benchmark_name = "Perf:hash_map_get_value";
    const size_t num_runs = NUM_RUNS;

    BENCHMARK_RUN_MULTIPLE(num_runs, benchmark_name, avg_runs_benchmark);

    // Calculate the average time and average success rate
    size_t avg_found_count = total_found_count / num_runs;
    LOG_SUCCESS("Average UUIDs found: %zu out of %zu\n", avg_found_count, num_runs);
}