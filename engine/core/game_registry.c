#include "game_registry.h"

#include "logging/log.h"

#include "gameobject.h"

hash_map_t* gGameRegistry = NULL;
uint32_t gNumObjects = 0;

void game_registry_init(void)
{
    gNumObjects = 0;
    // create the hash_map
    gGameRegistry = hash_map_create(MAX_OBJECTS);
}

void game_registry_destroy(void)
{
    hash_map_destroy(gGameRegistry);
}

random_uuid_t game_registry_register_gameobject_type(const char* typeName, uint32_t gameObjectDataSize, StartFunction StartFn, UpdateFunction UpdateFn)
{
    random_uuid_t uuid = {{ 0, 0, 0, 0 }};

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

    hash_map_set_key_value(gGameRegistry, uuid, (void*)game_object);

    gNumObjects++;
    return uuid;  // Return the UUID of the registered object
}

void game_registry_unregister_gameobject_type(const random_uuid_t uuid)
{
    GameObject* game_object = hash_map_get_value(gGameRegistry, uuid);
    if (game_object->gameObjectData)
        free(game_object->gameObjectData);
    hash_map_remove_entry(gGameRegistry, game_object->uuid);
}

GameObject* game_registry_get_gameobject_by_uuid(random_uuid_t goUUID)
{
    GameObject* go = (GameObject*)hash_map_get_value(gGameRegistry, goUUID);
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
