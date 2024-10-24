#include <stdio.h>
#include <string.h>

#include "test.h"

#include <engine/core/uuid/uuid.h>

// Test function
void test_uuid(void)
{
    const char* test_case = "test_uuid";

    // Generate a valid UUID
    {
        TEST_START();
#ifdef __GNUC__
        random_uuid_t uuid = {{0, 0, 0, 0}};
#else
        random_uuid_t uuid = {0, 0, 0, 0};
#endif
        uuid_generate(&uuid);
        TEST_END();
        ASSERT_CON(uuid_is_null(&uuid) == false, test_case, "UUID should not be null upon generation.");
    }

    // Subsequent generations should be unique
    {
        TEST_START();
        random_uuid_t uuid_1;
        random_uuid_t uuid_2;
        uuid_generate(&uuid_1);
        uuid_generate(&uuid_2);
        TEST_END();
        ASSERT_CON(uuid_compare(&uuid_1, &uuid_2) == 0, test_case, "UUIDs generated should not be same.");
    }

    // Testing copy + comparision
    {
        TEST_START();
        random_uuid_t uuid_1;
        random_uuid_t uuid_2;
        uuid_generate(&uuid_1);
        uuid_copy(&uuid_1, &uuid_2);
        TEST_END();
        ASSERT_CON(uuid_compare(&uuid_1, &uuid_2) == 1, test_case, "UUIDs copied should be same.");
    }
}
