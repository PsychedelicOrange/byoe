#ifndef GAME_REGISTRY_H
#define GAME_REGISTRY_H

#include <stddef.h>
#include <stdint.h>

#include "common.h"

#include "containers/hash_map.h"
#include "uuid/uuid.h"

#include "../scripting/scripting.h"

typedef struct GameObject GameObject;

/*******************************/
// GameObjects Registry
/*******************************/

// Initialize the game object registry
void game_registry_init(void);

// Cleanup the game object registry
void game_registry_destroy(void);

// Private methods to create game objects in the game world
random_uuid_t game_registry_register_gameobject_type(uint32_t gameObjectDataSize, StartFunction StartFn, UpdateFunction UpdateFn);

// Unregister and remove it from the game registry
void game_registry_unregister_gameobject_type(const random_uuid_t uuid);

// Returns the game object ptr stored in the registry
GameObject* game_registry_get_gameobject_by_uuid(random_uuid_t goUUID);

hash_map_t* game_registry_get_instance(void);

uint32_t game_registry_get_num_objects(void);

/*******************************/
// Macros for Registration & Instantiation
/*******************************/

// Macro for quick game object registration and spawning in the game world
#define REGISTER_GAME_OBJECT(DataType, StartFn, UpdateFn) \
    game_registry_register_gameobject_type(sizeof(DataType), StartFn, UpdateFn);

#define REGISTER_GAME_OBJECT_WITH_NODE_IDX(DataType, StartFn, UpdateFn, NodeIdx)                            \
                                                                                                                      \
    {                                                                                                                 \
        random_uuid_t goUUID = game_registry_register_gameobject_type(sizeof(DataType), StartFn, UpdateFn); \
        gameobject_set_sdf_node_idx(goUUID, NodeIdx);                                                                 \
    }

#define UNREGISTER_GAME_OBJECT(uuid) \
    game_registry_unregister_gameobject_type(uuid)

#endif    // GAME_REGISTRY_H