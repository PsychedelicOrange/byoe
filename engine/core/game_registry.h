#ifndef GAME_REGISTRY_H
#define GAME_REGISTRY_H

#include <stdint.h>

#include "common.h"

typedef void (*AutoRegisterFn)(); // Function pointer type for auto-register functions

AutoRegisterFn autoRegisterFunctions[MAX_OBJECTS];
extern int gAutoRegisterCount;

void registerAutoRegisterFunction(AutoRegisterFn func);
void autoRegisterGameObjects();

#endif // GAME_REGISTRY_H
