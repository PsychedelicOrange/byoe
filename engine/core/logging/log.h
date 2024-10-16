#ifndef LOG_H
#define LOG_H

#include <stdio.h>

// TODO: Add timestamp
// TODO: Add custom stream to write (ex. file, network, etc.)

#define LOG_SUCCESS(...) do { \
    printf(COLOR_GREEN "[BYOE] - SUCCESS: "); \
    printf(__VA_ARGS__); \
    printf(COLOR_RESET); \
} while(0)

#define LOG_INFO(...) do { \
    printf(COLOR_BLUE"[BYOE] - INFO: "); \
    printf(__VA_ARGS__); \
    printf(COLOR_RESET); \
} while(0)

#define LOG_WARN(...) do { \
    printf(COLOR_YELLOW "[BYOE] - WARNING: "); \
    printf(__VA_ARGS__); \
    printf("\033[0m\n"); \
} while(0)

#define LOG_ERROR(...) do { \
    printf(COLOR_RED "[BYOE] - ERROR: "); \
    printf(__VA_ARGS__); \
    printf(COLOR_RESET); \
} while(0)

#endif