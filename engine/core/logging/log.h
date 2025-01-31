#ifndef LOG_H
#define LOG_H

#include "common.h"
#include <stdio.h>

#include "common.h"

// TODO: Add timestamp
// TODO: Add custom stream to write (ex. file, network, etc.)

#define LOG_SUCCESS(...)                          \
    do {                                          \
        printf(COLOR_GREEN "[BYOE] - SUCCESS: "); \
        printf(__VA_ARGS__);                      \
        printf(COLOR_RESET);                      \
        printf("\n");                             \
    } while (0)

#define LOG_INFO(...)                         \
    do {                                      \
        printf(COLOR_BLUE "[BYOE] - INFO: "); \
        printf(__VA_ARGS__);                  \
        printf(COLOR_RESET);                  \
        printf("\n");                         \
    } while (0)

#define LOG_WARN(...)                                       \
    do {                                                    \
        fprintf(stderr, COLOR_YELLOW "[BYOE] - WARNING: "); \
        printf(__VA_ARGS__);                                \
        printf(COLOR_RESET);                                \
    } while (0)

#define LOG_ERROR(...)                                 \
    do {                                               \
        fprintf(stderr, COLOR_RED "[BYOE] - ERROR: "); \
        printf(__VA_ARGS__);                           \
        printf(COLOR_RESET);                           \
    } while (0)

#endif
