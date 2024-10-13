#include <stdio.h>
#include <string.h>

#include "test.h"

#include <engine/core/containers/hash_map.h>

// Helper function for displaying key-value pairs
void print_map(hash_map_t* map) {
    printf(COLOR_BLUE "Hash Map Contents:\n" COLOR_RESET);
    if (!map)
        return;
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries && map->entries[i].key && map->entries[i].value)
            printf("Key: %s, Value: %s\n", map->entries[i].key, (char*)map->entries[i].value);
    }
    printf("\n");
}

// Test function
void test_hash_map(void) {
    const char* test_case = "test_hash_map";

    hash_map_t* map = NULL;

    // Create a hash map
    {
        TEST_START();
        map = hash_map_create(5);
        TEST_END();
        ASSERT_EQ((size_t)0, hash_map_get_length(map), "%zu", test_case, "Map should be empty upon creation.");
    }

    // Test inserting some key-value pairs
    {
        TEST_START();
        hash_map_set_key_value(map, "apple", "fruit");
        hash_map_set_key_value(map, "carrot", "vegetable");
        hash_map_set_key_value(map, "pear", "fruit");
        hash_map_set_key_value(map, "broccoli", "vegetable");
        TEST_END();
        ASSERT_EQ((size_t)4, hash_map_get_length(map), "%zu", test_case, "Map should have 4 elements after insertion.");
    }

    // Test retrieval
    {
        TEST_START();
        const char* apple_val = (const char*)hash_map_get_value(map, "apple");
        TEST_END();
        ASSERT_STR_EQ("fruit", apple_val, test_case, "'apple' should map to 'fruit'.");
    }

    {
        TEST_START();
        const char* broccoli_val = (const char*)hash_map_get_value(map, "broccoli");
        TEST_END();
        ASSERT_STR_EQ("vegetable", broccoli_val, test_case, "'broccoli' should map to 'vegetable'.");
    }

    // Test removing an entry
    {
        TEST_START();
        hash_map_remove_entry(map, "carrot");
        TEST_END();
        ASSERT_EQ((size_t)3, hash_map_get_length(map), "%zu", test_case, "'carrot' should be removed, reducing length to 3.");
    }

        {
        TEST_START();
        hash_map_remove_entry(map, "apple");
        TEST_END();
        ASSERT_EQ((size_t)2, hash_map_get_length(map), "%zu", test_case, "'apple' should be removed, reducing length to 3.");
    }

    // Iterate over the hash map using the iterator and compare with entries,
    // to make sure hash_map_parse_next works properly as intended
    {
        TEST_START();
        hash_map_iterator_t iter = hash_map_iterator_begin(map);
        size_t iter_count = 0;
        while (hash_map_parse_next(&iter)) {
            iter_count++;
        }
        TEST_END();
        ASSERT_EQ((size_t)2, iter_count, "%zu", test_case, "Iterator should find exactly 3 elements.");
    }

    // Clean up
    hash_map_destroy(map);
    map = NULL;
}
