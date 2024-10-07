#ifndef GAMEOBJECTS_H
#define GAMEOBJECTS_H

// std.
#include <stdint.h>

// external
#include <cglm/cglm.h>

// proj
#include "../scripting/scripting.h"

// Manimum number og game Object Instances in the game world at any moment
#define MAX_OBJECTS 1024

typedef struct GameObject
{
    const char* name; // Name of the game object
    const char* typeName; // Type of the game object (typeid)
    void* gameState; // Storead global game state such as Input, world info, scene data etc. 
    void* gameObjectData; // game object specific data that can be serialized/reflected
    StartFunction startFn;
    UpdateFunction updateFn;
    // TODO: Add collision callback functions here if needed
} GameObject;

static vec3 gOrigin = {0.0f, 0.0f, 9.0f};

// TODO: Make this a hash_map
static GameObject gGameRegistry[MAX_OBJECTS];
static uint32_t gNumObjects = 0;

/*******************************/
// GameObjects Registry
/*******************************/
// Private methods to create game objects in the game world
const GameObject* registerGameObjectType(const char* typeName, void* gameObjectData, StartFunction start, UpdateFunction update);

void unRegisterGameObjectType(GameObject* obj);

// Macro fgor quick game object registration and spawning in the game world
#define REGISTER_GAME_OBJECT(Type, gameObjectData, Start, Update)

// Instatiates a new game object by cloning "a" instance at the given position and rotation
#define INSTANTIATE_GAME_OBJECT(Obj,Pos, Rot)

#endif
