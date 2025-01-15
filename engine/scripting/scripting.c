#include "scripting.h"

#include "game_registry.h"
#include "game_state.h"
#include "gameobject.h"

#include "logging/log.h"

#include "../render/renderer_sdf.h"
#include "../scene/sdf_scene.h"

#include <limits.h>

void gameobjects_start(void)
{
    hash_map_t* registry = game_registry_get_instance();

    for (size_t i = 0; i < registry->capacity; i++) {
        hash_map_pair_t pair = registry->entries[i];
        if (!uuid_is_null(&pair.key) && pair.value) {
            GameObject* go = (GameObject*) pair.value;
            go->startFn(&go->uuid);
        }
    }
}

void gameobjects_update(float dt)
{
    (void) dt;
    hash_map_t* registry = game_registry_get_instance();

    const SDF_Scene* scene = renderer_sdf_get_scene();

    for (size_t i = 0; i < registry->capacity; i++) {
        hash_map_pair_t pair = registry->entries[i];
        if (!uuid_is_null(&pair.key) && pair.value) {
            GameObject* go = (GameObject*) pair.value;
            if (go->updateFn)
                go->updateFn(&go->uuid, dt);

            if (go->sdfNodeIdx >= 0 && go->sdfNodeIdx != UINT_MAX && go->isRenderable)
                scene->nodes[go->sdfNodeIdx].primitive.transform = go->transform;
        }
    }
}
