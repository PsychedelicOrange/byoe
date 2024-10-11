#ifndef GAMEOBJECTS_H
#define GAMEOBJECTS_H

// std.
#include <stdint.h>

// external
#include <cglm/cglm.h>

// proj
#include "common.h"
#include "../scripting/scripting.h"
#include "uuid/uuid.h"

#include "game_registry.h"

typedef struct GameObject
{
    uuid_t uuid;
    const char* typeName;       // Type of the game object (class/type)
    void* gameState;            // Global game state such as Input, world info, scene data etc. 
    void* gameObjectData;       // Object-specific data that can be serialized/reflected
    StartFunction startFn;      // Start function for the object
    UpdateFunction updateFn;    // Update function for the object
    // TODO: Add collision callback functions here if needed
} GameObject;

static vec3 gOrigin = {0.0f, 0.0f, 9.0f};

#endif
