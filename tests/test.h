#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <engine/core/common.h>


// Unicode characters for check mark and cross
#ifdef _WIN32
#define UNICODE_CHECKMARK "_/"  // ✅
#define UNICODE_CROSS     "x"  // ❌
#else
#define UNICODE_CHECKMARK "\xE2\x9C\x85"  // ✅
#define UNICODE_CROSS     "\xE2\x9D\x8C"  // ❌
#endif

#define TEST_START() \
    clock_t start_time = clock();

#define TEST_END() \
    clock_t end_time = clock(); \
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

// Generic assert for conditions
#define ASSERT_CON(condition, test_case, msg) \
    if (condition) { \
        printf(COLOR_GREEN "[Test Case] Passed %s : %s : STATS: (speed: %.3f ms) : %s\n" COLOR_RESET, \
            UNICODE_CHECKMARK, test_case, time_spent * 1000, msg); \
    } else { \
        printf(COLOR_RED "[Test Case] Failed %s : %s : " COLOR_RESET "REASON: %s\n", \
            UNICODE_CROSS, test_case, msg); \
    }

// Generic assert for non-string types, passing format specifier
#define ASSERT_EQ(expected, actual, fmt, test_case, msg) \
    if ((expected) != (actual)) { \
        printf(COLOR_RED "[Test Case] Failed %s : %s : " COLOR_RESET "Expected " fmt " but got " fmt " | REASON: %s\n", \
               UNICODE_CROSS, test_case, expected, actual, msg); \
    } else { \
        printf(COLOR_GREEN "[Test Case] Passed %s : %s : STATS: (speed: %.3f ms) : %s\n" COLOR_RESET, \
               UNICODE_CHECKMARK, test_case, time_spent * 1000, msg); \
    }

// Specialized assert for string comparison
#define ASSERT_STR_EQ(expected, actual, test_case, msg) \
    if (strcmp((expected), (actual)) != 0) { \
        printf(COLOR_RED "[Test Case] Failed %s : %s : " COLOR_RESET "Expected '%s' but got '%s' | REASON: %s\n", \
               UNICODE_CROSS, test_case, expected, actual, msg); \
    } else { \
        printf(COLOR_GREEN "[Test Case] Passed %s : %s : STATS: (speed: %.3f ms) : %s\n" COLOR_RESET, \
               UNICODE_CHECKMARK, test_case, time_spent * 1000, msg); \
    }

#endif // TEST_H
