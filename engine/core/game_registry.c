#include "game_registry.h"

#include "logging/log.h"

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
    hash_map_destroy(gGameRegistry);
}

uuid_t register_gameobject_type(const char* typeName, uint32_t gameObjectDataSize, StartFunction StartFn, UpdateFunction UpdateFn)
{
    uuid_t uuid = {{ 0, 0, 0, 0 }};

    if (gNumObjects >= MAX_OBJECTS) {
        LOG_ERROR("Registry FULL cannot create new game objects!");
        return uuid;
    }

    uuid_generate(&uuid);

    GameObject* game_object = malloc(sizeof(GameObject));
    uuid_copy(&uuid, &game_object->uuid);

    // Init transform to 0
    vec3 pos = { 0, 0, 0 };
    glm_vec3_copy(pos, game_object->transform.position);
    vec3 scale = { 1, 1, 1 };
    glm_vec3_copy(scale, game_object->transform.scale);
    versor rotquat = { 0, 0, 0, 0 };
    glm_quat_copy(rotquat, game_object->transform.rotation);

    strcpy(game_object->typeName, typeName);
    if (gameObjectDataSize > 0)
        game_object->gameObjectData = malloc(gameObjectDataSize);
    game_object->startFn = StartFn;
    game_object->updateFn = UpdateFn;

    hash_map_set_key_value(gGameRegistry, (const char*)uuid.data, (void*)game_object);

    gNumObjects++;
    return uuid;  // Return the UUID of the registered object
}

void unregister_gameobject_type(const uuid_t uuid)
{
    GameObject* game_object = hash_map_get_value(gGameRegistry, (const char*)uuid.data);
    if (game_object->gameObjectData)
        free(game_object->gameObjectData);
    hash_map_remove_entry(gGameRegistry, (const char*)&game_object->uuid);
}

GameObject* get_gameobject_by_uuid(uuid_t goUUID)
{
    GameObject* go = (GameObject*)hash_map_get_value(gGameRegistry, (const char*)goUUID.data);
    if (go != NULL) {
        return go;
    }
    else {
        LOG_ERROR("[Game Registry] cannot find gameobject using uuid! | UUID: %s\n", uuid_to_string(&goUUID));
        return NULL;
    }
}


hash_map_t* game_registry_get_instance(void)
{
    return gGameRegistry;
}

uint32_t game_registry_get_num_objects(void)
{
    return gNumObjects;
}
