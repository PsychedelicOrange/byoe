#include "game_registry.h"

#include "log/log.h"

#include "gameobject.h"

hash_map_t* gGameRegistry = NULL;
uint32_t gNumObjects = 0;

void init_game_registry(void)
{
    gNumObjects = 0;
    // create the hash_map
    gGameRegistry = hash_map_create(MAX_OBJECTS);
}

void cleanup_game_registry(void)
{
    // TODO: clean out and destroy the hash table and unregister everything
    hash_map_destroy(gGameRegistry);
}

uuid_t register_gameobject_type(const char* typeName, uint32_t gameObjectDataSize, StartFunction StartFn, UpdateFunction UpdateFn)
{
    uuid_t uuid = {0, 0, 0, 0};

    if (gNumObjects >= MAX_OBJECTS) {
        LOG_ERROR("Registry FULL cannot create new game objects!");
        return uuid; 
    }

    uuid_generate(&uuid);  // Generate a UUID key
    uuid_print(&uuid);

    GameObject game_object;
    game_object.typeName = typeName;
    game_object.gameObjectData = malloc(gameObjectDataSize);
    game_object.startFn = StartFn;
    game_object.updateFn = UpdateFn;

    hash_map_set_key_value(gGameRegistry, (const char*)&uuid, (void*)&game_object);

    gNumObjects++;
    return uuid;  // Return the UUID of the registered object
}

void unregister_gameobject_type(const uuid_t uuid) {
    GameObject* game_object = hash_map_get_value(gGameRegistry, (const char*)&uuid);
    free(game_object->gameObjectData);
    hash_map_remove_entry(gGameRegistry, (const char*)&game_object->uuid);
}