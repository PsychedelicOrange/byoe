#include <stdio.h>
#include <string.h>

#include "test.h"

#include <engine/core/rng/rng.h>

// Test function for RNG
void test_rng(void) {
    const char* test_case = "test_rng";

    // Test generation of several random numbers
    {
        TEST_START();
        uint32_t random_val1 = rng_generate();
        uint32_t random_val2 = rng_generate();
        TEST_END();
        
        ASSERT_CON(random_val1 != random_val2, test_case, "Two consecutive random numbers should not be equal.");
    }

    // Test generating a random number within a range
    {
        TEST_START();
        uint32_t random_val = rng_range(10, 20);
        TEST_END();
        
        ASSERT_CON(random_val >= 10, test_case, "Random number should be greater than or equal to 10.");
        ASSERT_CON(random_val <= 20, test_case, "Random number should be less than or equal to 20.");
    }

    // Test range with a single possible value (min == max)
    {
        TEST_START();
        uint32_t random_val = rng_range(5, 5);
        TEST_END();

        ASSERT_EQ(random_val, 5, "%u", test_case, "Random number should always be 5 when min equals max.");
    }
}