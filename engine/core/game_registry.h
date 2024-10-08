#ifndef GAME_REGISTRY_H
#define GAME_REGISTRY_H

#include <stdint.h>

#include "common.h"

typedef void (*AutoRegisterFn)(); // Function pointer type for auto-register functions

AutoRegisterFn autoRegisterFunctions[MAX_OBJECTS];
static int gAutoRegisterCount;

static void autoRegisterGameObjects()
{
    printf("autoregister count : %d", gAutoRegisterCount);

    // Call all auto-register functions
    for (int i = 0; i < gAutoRegisterCount; i++) {
        autoRegisterFunctions[i](); // Call the registered auto-register function
    }

}

#endif // GAME_REGISTRY_H
