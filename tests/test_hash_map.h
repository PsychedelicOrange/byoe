#include <stdio.h>
#include <string.h>

#include "test.h"

#include <engine/core/containers/hash_map.h>
#include <engine/core/logging/log.h>
#include <engine/core/uuid/uuid.h>


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

    random_uuid_t uuid_retriieve_1;
    random_uuid_t uuid_retriieve_2;

   // Test inserting some key-value pairs
   {
        TEST_START();
        random_uuid_t uuid;
        
        uuid_generate(&uuid);
        hash_map_set_key_value(map, uuid, "apple");
        
        uuid_generate(&uuid_retriieve_1);
        hash_map_set_key_value(map, uuid_retriieve_1, "carrot");
        
        uuid_generate(&uuid_retriieve_2);
        hash_map_set_key_value(map, uuid_retriieve_2, "pear");
        
        uuid_generate(&uuid);
        hash_map_set_key_value(map, uuid, "broccoli");
        TEST_END();
        ASSERT_EQ((size_t)4, hash_map_get_length(map), "%zu", test_case, "Map should have 4 elements after insertion.");
   }

   // Test retrieval
   {
       TEST_START();
       const char* ret_val_1 = (const char*)hash_map_get_value(map, uuid_retriieve_1);
       TEST_END();
       ASSERT_STR_EQ("carrot", ret_val_1, test_case, "uuid should map to 'carrot'.");
   }

   {
       TEST_START();
       const char* ret_val_2 = (const char*)hash_map_get_value(map, uuid_retriieve_2);
       TEST_END();
       ASSERT_STR_EQ("pear", ret_val_2, test_case, "uuid should map to 'pear'.");
   }

   // Test removing an entry
   {
       TEST_START();
       hash_map_remove_entry(map, uuid_retriieve_2);
       TEST_END();
       ASSERT_EQ((size_t)3, hash_map_get_length(map), "%zu", test_case, "key/value pair should be removed, reducing length to 3.");
   }

   {
       TEST_START();
       hash_map_remove_entry(map, uuid_retriieve_1);
       TEST_END();
       ASSERT_EQ((size_t)2, hash_map_get_length(map), "%zu", test_case, "key/value pair should be removed, reducing length to 2.");
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
       ASSERT_EQ((size_t)2, iter_count, "%zu", test_case, "Iterator should find exactly 2 elements.");
   }

   // Clean up
   hash_map_destroy(map);
   map = NULL;
}
