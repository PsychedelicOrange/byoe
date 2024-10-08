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

// TODO: Make this a hash_map
typedef struct HashMapNode {
    const char* key;           // GameObject type name as the key
    GameObject value;          // GameObject instance
    bool occupied;             // Is this slot occupied?
} HashMapNode;

static HashMapNode* gGameRegistry = NULL;
static uint32_t gNumObjects = 0;

/*******************************/
// HashMap Helpers
/*******************************/

// Simple hash function to reduce UUID to index
static uint32_t hashUUID(const uuid_t uuid) {
    uint32_t hash = 0;
    // Iterate over the data array within the uuid_t structure
    for (int i = 0; i < 4; i++) {
        hash += uuid.data[i]; // Access each 32-bit part of the UUID
    }
    return hash % MAX_OBJECTS; // Ensure the hash fits within the MAX_OBJECTS
}

// Find the index of a free spot or an existing UUID in the registry
static int findFreeSpotOrUUID(const uuid_t uuid, bool findExisting) {
    uint32_t index = hashUUID(uuid);
    uint32_t startIndex = index;
    while (gGameRegistry[index].occupied) {
        if (findExisting && uuid_compare((uuid_t*)gGameRegistry[index].key, &uuid) == 0) {
            return index;  // Found the UUID in the registry
        }
        index = (index + 1) % MAX_OBJECTS;
        if (index == startIndex) return -1;  // No free spot or UUID not found
    }
    return findExisting ? -1 : index;  // Free spot found or UUID not found
}

// Simple hash function (djb2)
static uint32_t hash(const char* key) {
    uint32_t hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;
    return hash % MAX_OBJECTS;
}

// Find the index of the key or return the next available spot
static int findNodeIndex(const char* key) {
    uint32_t index = hash(key);
    uint32_t startIndex = index;
    while (gGameRegistry[index].occupied && strcmp(gGameRegistry[index].key, key) != 0) {
        index = (index + 1) % MAX_OBJECTS;
        if (index == startIndex) return -1;  // Table is full
    }
    return index;
}

/*******************************/
// GameObjects Registry
/*******************************/

// Initialize the game object registry
static void initGameRegistry() {
    gGameRegistry = (HashMapNode*)calloc(MAX_OBJECTS, sizeof(HashMapNode));
    gNumObjects = 0;
}

// Cleanup the game object registry
static void cleanupGameRegistry() {
    for (uint32_t i = 0; i < MAX_OBJECTS; ++i) {
        if (gGameRegistry[i].occupied && gGameRegistry[i].value.gameObjectData) {
            free(gGameRegistry[i].value.gameObjectData);  // Free allocated game object data
        }
    }
    free(gGameRegistry);
    gGameRegistry = NULL;
    gNumObjects = 0;
}


// Private methods to create game objects in the game world
uuid_t registerGameObjectType(const char* typeName, uint32_t gameObjectDataSize, StartFunction StartFn, UpdateFunction UpdateFn);

void unRegisterGameObjectType(const uuid_t uuid);

/*******************************/
// Macros for Registration & Instantiation
/*******************************/

// Macro for quick game object registration and spawning in the game world
#define REGISTER_GAME_OBJECT(UUID, TypeName, DataType, StartFn, UpdateFn) \
    static void register_##TypeName() { \
        UUID = registerGameObjectType(#TypeName, sizeof(DataType), StartFn, UpdateFn); \
    } \
    __attribute__((constructor)) static void autoRegister_##TypeName() { \
        registerAutoRegisterFunction(register_##TypeName); \
    }
    

#define UNREGISTER_GAME_OBJECT(uuid) \
    unregisterGameObject(uuid)

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
