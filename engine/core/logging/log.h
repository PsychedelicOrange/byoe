#ifndef LOG_H
#define LOG_H

#include <stdio.h>

// TODO: Add timestamp
// TODO: Add custom stream to write (ex. file, network, etc.)

#define LOG_SUCCESS(...) do { \
    printf("\033[1;32m[BYOE] - SUCCESS: "); \
    printf(__VA_ARGS__); \
    printf("\033[0m\n"); \
} while(0)

#define LOG_INFO(...) do { \
    printf("\033[1;37m[BYOE] - INFO: "); \
    printf(__VA_ARGS__); \
    printf("\033[0m\n"); \
} while(0)

#define LOG_WARN(...) do { \
    printf("\033[1;33m[BYOE] - WARNING: "); \
    printf(__VA_ARGS__); \
    printf("\033[0m\n"); \
} while(0)

#define LOG_ERROR(...) do { \
    printf("\033[1;31m[BYOE] - ERROR: "); \
    printf(__VA_ARGS__); \
    printf("\033[0m\n"); \
} while(0)

#endif