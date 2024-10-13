#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"

#define TEST_START() \
    clock_t start_time = clock();

#define TEST_END() \
    clock_t end_time = clock(); \
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

#define ASSERT_EQ(expected, actual, test_case, msg) \
    if ((expected) != (actual)) { \
        printf(COLOR_RED "[Test Case] Failed ❌ : %s : " COLOR_RESET "Expected %s but got %s | REASON: %s\n", \
               test_case, #expected, #actual, msg); \
        exit(EXIT_FAILURE); \
    } else { \
        printf(COLOR_GREEN "[Test Case] Passed ✅ : %s : STATS: (speed: %.3f ms) : %s\n" COLOR_RESET, \
               test_case, time_spent * 1000, msg); \
    }

#define ASSERT_STR_EQ(expected, actual, test_case, msg) \
    if (strcmp((expected), (actual)) != 0) { \
        printf(COLOR_RED "[Test Case] Failed ❌ : %s : " COLOR_RESET "Expected '%s' but got '%s' | REASON: %s\n", \
               test_case, expected, actual, msg); \
        exit(EXIT_FAILURE); \
    } else { \
        printf(COLOR_GREEN "[Test Case] Passed ✅ : %s : STATS: (speed: %.3f ms) : %s\n" COLOR_RESET, \
               test_case, time_spent * 1000, msg); \
    }

#endif // TEST_H
