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

#define MAYBE_UNUSED __attribute__((unused))

// Maximum number of game Object Instances in the game world at any moment
#define MAX_OBJECTS 256

#define COLOR_RESET  "\x1b[0m"
#define COLOR_RED    "\x1b[31m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE   "\x1b[34m"
#define COLOR_PINK   "\x1b[35m"

#ifdef SHADER_INCLUDE
    #define ENUM(name, ...) enum name \
    {                                 \
        __VA_ARGS__                   \
    };
    #define STRUCT(name, ...) \
        struct name           \
        {                     \
            __VA_ARGS__       \
        };
#else
    #define ENUM(name, ...) typedef enum name \
    {                                         \
        __VA_ARGS__                           \
    } name;
    #define STRUCT(name, ...) \
        typedef struct name   \
        {                     \
            __VA_ARGS__       \
        } name;
#endif

#define DEFINE_CLAMP(type)               \
    static type clamp_##type(type value, \
        type                      min,   \
        type                      max)   \
    {                                    \
        if (value < min) {               \
            return min;                  \
        } else if (value > max) {        \
            return max;                  \
        } else {                         \
            return value;                \
        }                                \
    }

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])

#define SAFE_FREE(x) \
    if (x) {         \
        free(x);     \
        x = NULL;    \
    }
// DEFINE_CLAMP(float)