#ifndef GAME_REGISTRY_H
#define GAME_REGISTRY_H

#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "../scripting/scripting.h"
#include "uuid/uuid.h"
#include "containers/hash_map.h"

/*******************************/
// GameObjects Registry
/*******************************/

extern hash_map_t gGameRegistry[MAX_OBJECTS];
extern uint32_t gNumObjects;

// Initialize the game object registry
void init_game_registry();

// Cleanup the game object registry
void cleanup_game_registry();

// Private methods to create game objects in the game world
uuid_t register_gameobject_type(const char* typeName, uint32_t gameObjectDataSize, StartFunction StartFn, UpdateFunction UpdateFn);

// Unregister and remove it from the game registry
void unregister_gameobject_type(const uuid_t uuid);

/*******************************/
// Macros for Registration & Instantiation
/*******************************/

// Macro for quick game object registration and spawning in the game world
#define REGISTER_GAME_OBJECT(UUID, TypeName, DataType, StartFn, UpdateFn) \
        UUID = register_gameobject_type(#TypeName, sizeof(DataType), StartFn, UpdateFn);
    

#define UNREGISTER_GAME_OBJECT(uuid) \
    unregister_gameobject_type(uuid)

// FIXME:
// Macro to instantiate a game object by cloning an existing template
#define INSTANTIATE_GAME_OBJECT(Obj, Pos, Rot) ({ \
    int index = findNodeIndex(Obj->typeName); \
    if (index == -1 || !gGameRegistry[index].occupied) { \
        NULL; /* Object not found */ \
    } else { \
        GameObject* obj = &gGameRegistry[gNumObjects++].value; \
        *obj = *Obj; /* Clone the object */ \
        glm_vec3_copy((Pos), obj->position); \
        glm_quat_copy((Rot), obj->rotation); \
        if (obj->startFn) obj->startFn(obj); \
        obj; /* Return the newly instantiated object */ \
    } \
})



#endif