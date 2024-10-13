#include <stdio.h>
#include <string.h>

#include "test.h"

#include <engine/core/uuid/uuid.h>

// Test function
void test_uuid(void) {
    const char* test_case = "test_uuid";

    // Generate a valid UUID
    {
        TEST_START();
        uuid_t uuid = {0, 0, 0, 0};
        uuid_generate(&uuid);
        TEST_END();
        ASSERT_CON(uuid_is_null(&uuid), test_case, "UUID should not be null upon generation.");
    }

    // Subsequent generations should be unique
    {
        TEST_START();
        uuid_t uuid_1;
        uuid_t uuid_2;
        uuid_generate(&uuid_1);
        uuid_generate(&uuid_2);
        TEST_END();
        ASSERT_CON(uuid_compare(&uuid_1, &uuid_2) == 1, test_case, "UUIDs generated should not be same.");
    }

    // Testing copy + comparision
    {
        TEST_START();
        uuid_t uuid_1;
        uuid_t uuid_2;
        uuid_generate(&uuid_1);
        uuid_copy(&uuid_1, &uuid_2);
        TEST_END();
        ASSERT_CON(uuid_compare(&uuid_1, &uuid_2) == 0, test_case, "UUIDs copied should be same.");
    }
}
