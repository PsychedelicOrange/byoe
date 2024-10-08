#include "game_registry.h"

#include <stdio.h>

int gAutoRegisterCount = 0;

void registerAutoRegisterFunction(AutoRegisterFn func) {
    if (gAutoRegisterCount < MAX_OBJECTS) {
        autoRegisterFunctions[gAutoRegisterCount++] = func;
                printf("autoregistercount : %d", gAutoRegisterCount);
    }
}

void autoRegisterGameObjects()
{
    printf("autoregister count : %d", gAutoRegisterCount);

    // Call all auto-register functions
    for (int i = 0; i < gAutoRegisterCount; i++) {
        autoRegisterFunctions[i](); // Call the registered auto-register function
    }

}