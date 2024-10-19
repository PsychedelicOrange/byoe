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
void init_game_registry(void);

// Cleanup the game object registry
void cleanup_game_registry(void);

// Private methods to create game objects in the game world
random_uuid_t register_gameobject_type(const char* typeName, uint32_t gameObjectDataSize, StartFunction StartFn, UpdateFunction UpdateFn);

// Unregister and remove it from the game registry
void unregister_gameobject_type(const random_uuid_t uuid);

// Returns the game object ptr stored in the registry 
GameObject* get_gameobject_by_uuid(random_uuid_t goUUID);

hash_map_t* game_registry_get_instance(void);

uint32_t game_registry_get_num_objects(void);

/*******************************/
// Macros for Registration & Instantiation
/*******************************/

// Macro for quick game object registration and spawning in the game world
#define REGISTER_GAME_OBJECT(TypeName, DataType, StartFn, UpdateFn) \
        register_gameobject_type(TypeName, sizeof(DataType), StartFn, UpdateFn);


#define UNREGISTER_GAME_OBJECT(uuid) \
    unregister_gameobject_type(uuid)

#endif // GAME_REGISTRY_H