#pragma once

// Windows-specific export/import macros
#ifdef _WIN32
    #ifdef BUILD_SHARED_ENGINE
        #define BYOE_API __declspec(dllexport)
    #else
        #define BYOE_API __declspec(dllimport)
    #endif
#else
    // For GCC/Clang, use visibility attribute
    #ifdef BUILD_SHARED_ENGINE
        #define BYOE_API __attribute__((visibility("default")))
    #else
        #define BYOE_API
    #endif
#endif

// Maximum number of game Object Instances in the game world at any moment
#define MAX_OBJECTS 1024

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
