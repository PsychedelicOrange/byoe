#include <stdio.h>
#include <string.h>

#include "test.h"
#include "benchmark.h"

#include <engine/core/containers/hash_map.h>
#include <engine/core/logging/log.h>
#include <engine/core/uuid/uuid.h>


// Test function
//void test_hash_map(void) {
//    const char* test_case = "test_hash_map";
//
//    hash_map_t* map = NULL;
//
//    // Create a hash map
//    {
//        TEST_START();
//        map = hash_map_create(5);
//        TEST_END();
//        ASSERT_EQ((size_t)0, hash_map_get_length(map), "%zu", test_case, "Map should be empty upon creation.");
//    }
//
//    // Test inserting some key-value pairs
//    {
//        TEST_START();
//        hash_map_set_key_value(map, "apple", "fruit");
//        hash_map_set_key_value(map, "carrot", "vegetable");
//        hash_map_set_key_value(map, "pear", "fruit");
//        hash_map_set_key_value(map, "broccoli", "vegetable");
//        TEST_END();
//        ASSERT_EQ((size_t)4, hash_map_get_length(map), "%zu", test_case, "Map should have 4 elements after insertion.");
//    }
//
//    // Test retrieval
//    {
//        TEST_START();
//        const char* apple_val = (const char*)hash_map_get_value(map, "apple");
//        TEST_END();
//        ASSERT_STR_EQ("fruit", apple_val, test_case, "'apple' should map to 'fruit'.");
//    }
//
//    {
//        TEST_START();
//        const char* broccoli_val = (const char*)hash_map_get_value(map, "broccoli");
//        TEST_END();
//        ASSERT_STR_EQ("vegetable", broccoli_val, test_case, "'broccoli' should map to 'vegetable'.");
//    }
//
//    // Test removing an entry
//    {
//        TEST_START();
//        hash_map_remove_entry(map, "carrot");
//        TEST_END();
//        ASSERT_EQ((size_t)3, hash_map_get_length(map), "%zu", test_case, "'carrot' should be removed, reducing length to 3.");
//    }
//
//    {
//        TEST_START();
//        hash_map_remove_entry(map, "apple");
//        TEST_END();
//        ASSERT_EQ((size_t)2, hash_map_get_length(map), "%zu", test_case, "'apple' should be removed, reducing length to 2.");
//    }
//
//    // Iterate over the hash map using the iterator and compare with entries,
//    // to make sure hash_map_parse_next works properly as intended
//    {
//        TEST_START();
//        hash_map_iterator_t iter = hash_map_iterator_begin(map);
//        size_t iter_count = 0;
//        while (hash_map_parse_next(&iter)) {
//            iter_count++;
//        }
//        TEST_END();
//        ASSERT_EQ((size_t)2, iter_count, "%zu", test_case, "Iterator should find exactly 2 elements.");
//    }
//
//    // Clean up
//    hash_map_destroy(map);
//    map = NULL;
//}

char* words[] = {
    "apple", "banana", "cherry", "date", "elderberry",
    "fig", "grape", "honeydew", "kiwi", "lemon",
    "mango", "nectarine", "orange", "papaya", "quince",
    "raspberry", "strawberry", "tangerine", "dragonfruit", "watermelon"
};

char* get_random_word() {
    int num_words = sizeof(words) / sizeof(words[0]);
    int random_index = rand() % num_words;  // Get a random index
    return words[random_index];
}

#define RAND_STR_LEN 1024 * 4

#define NUM_RUNS 1024 * 4

void benchmark_hash_map(void)
{
    const size_t STEPS = RAND_STR_LEN;

    setvbuf(stdout, NULL, _IOFBF, 1024);  // Fully buffered with a 1KB buffer
    setbuf(stdout, NULL);

    hash_map_t* map = NULL;
    map = hash_map_create(STEPS);


    srand((unsigned int)time(NULL));  // Seed for random number generator

    //---------------------------------------
    //{
    //    const char* benchmark_name = "Generate Random Words";
    //    BENCHMARK_START(benchmark_name);
    //    for (size_t i = 0; i < STEPS; i++)
    //    {
    //        LOG_INFO("Random String: %s\n", get_random_word());
    //    }
    //    BENCHMARK_END(benchmark_name);
    //}

    //---------------------------------------
    // create a bunch of UUIDs and add the English alphabet as the value

    //{
    //    const char* benchmark_name = "Set Key(UUID) Value(Random Word) pair";
    //    BENCHMARK_START(benchmark_name);
    //    for (size_t i = 0; i < STEPS; i++)
    //    {
    //        random_uuid_t uuid;
    //        uuid_generate(&uuid);
    //        hash_map_set_key_value(map, uuid, get_random_word());
    //    }
    //    BENCHMARK_END(benchmark_name);

    //    hash_map_print(map);
    //    hash_map_destroy(map);
    //}

    //---------------------------------------
    // retrieve each UUID

    //{
    //    map = hash_map_create(STEPS);
    //    const char* benchmark_name = "Get Value(Random Word) and Verify";
    //    BENCHMARK_START(benchmark_name);

    //    // Array to store UUIDs for later retrieval
    //    random_uuid_t* uuids = malloc(STEPS * sizeof(random_uuid_t));

    //    //hash_map_print(map);

    //    // Generate UUIDs and store them in the array while setting values
    //    for (size_t i = 0; i < STEPS; i++) {
    //        uuid_generate(&uuids[i]);
    //        hash_map_set_key_value(map, uuids[i], get_random_word());
    //    }

    //    //hash_map_print(map);

    //    // Retrieve and verify each UUID
    //    size_t found_count = 0;
    //    for (size_t i = 0; i < STEPS; i++) {
    //        //LOG_INFO("finding key... with uuid: %s\n", uuid_to_string(&uuids[i]));
    //        void* value = hash_map_get_value(map, uuids[i]);
    //        if (value != NULL) {
    //            found_count++;
    //            //LOG_SUCCESS("Retrieved Value for index: %zu | key: %s | Value: %s!\n", i, uuid_to_string(&uuids[i]), (char*)value);

    //        }
    //        else {
    //            //LOG_ERROR("Value not found for index: %zu | key: %s | Value: %s!\n", i, uuid_to_string(&uuids[i]), (char*)value);
    //        }
    //    }

    //    // Verify that all UUIDs were found
    //    if (found_count == STEPS) {
    //        LOG_SUCCESS("All %zu UUIDs found successfully!\n", STEPS);
    //    }
    //    else {
    //        LOG_ERROR("Failed to find %zu UUIDs.\n", STEPS - found_count);
    //    }

    //    free(uuids); // Clean up allocated memory
    //    BENCHMARK_END(benchmark_name);
    ////}

    {
        const size_t num_runs = NUM_RUNS;  // Number of times to run the benchmark
        double total_time = 0.0;    // To accumulate total time over all runs
        size_t total_found_count = 0; // To accumulate total found UUIDs over all runs

        for (size_t run = 0; run < num_runs; run++) {
            map = hash_map_create(STEPS);
            //const char* benchmark_name = "Get Value(Random Word) and Verify";
            BENCHMARK_START();

            // Array to store UUIDs for later retrieval
            random_uuid_t* uuids = malloc(STEPS * sizeof(random_uuid_t));

            // Generate UUIDs and store them in the array while setting values
            for (size_t i = 0; i < STEPS; i++) {
                uuid_generate(&uuids[i]);
                hash_map_set_key_value(map, uuids[i], get_random_word());
            }

            // Retrieve and verify each UUID
            size_t found_count = 0;
            for (size_t i = 0; i < STEPS; i++) {
                void* value = hash_map_get_value(map, uuids[i]);
                if (value != NULL) {
                    found_count++;
                }
            }

            // Accumulate found counts
            total_found_count += found_count;

            free(uuids); // Clean up allocated memory
            BENCHMARK_END();

            // Accumulate benchmark time for this run
            total_time += BENCHMARK_TIME();

            // Free the map after each run
            hash_map_destroy(map);
        }

        // Calculate the average time and average success rate
        double avg_time = total_time / num_runs;
        size_t avg_found_count = total_found_count / num_runs;

        // Print the final result
        LOG_SUCCESS("Benchmark completed %zu runs.\n", num_runs);
        LOG_SUCCESS("Average time: %.4f ms\n", avg_time);
        LOG_SUCCESS("Average UUIDs found: %zu out of %zu\n", avg_found_count, STEPS);
    }
}