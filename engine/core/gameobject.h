#ifndef GAMEOBJECTS_H
#define GAMEOBJECTS_H

// std.
#include <stdint.h>

#include "common.h"

// proj
#include "../scene/transform.h"
#include "../scripting/scripting.h"
#include "game_registry.h"
#include "uuid/uuid.h"

// Q. How to solve sending/updating transform, we don't have transform component and mesh data
// sol.1: we can manually link mesh to GameObject via registration or util functions in start itself, like game_object_set_mesh
// game_object_set_transform etc. we still need to pass it GameObject reference tho
// so we can replace the scripting functions to take the GameObject itself instead of limited data from this struct, kinda like
// emulating this pointer like behavior. We can hardcore stuff like Transform as it makes sense similar to uuid
// or we already have the UUID of the scripting functions for easy access of the below game object public API

typedef struct GameObject
{
    random_uuid_t  uuid;
    Transform      transform;
    bool           isRenderable;
    bool           _pad0[3];
    uint32_t       sdfNodeIdx;        // index of the sdf node in sdf scene array
    void*          gameObjectData;    // Object-specific data that can be serialized/reflected
    StartFunction  startFn;
    UpdateFunction updateFn;
    // TODO: Add collision callback functions here if needed
} GameObject;

void     gameobject_set_sdf_node_idx(random_uuid_t goUUID, uint32_t index);
uint32_t gameobject_get_sdf_node_idx(random_uuid_t goUUID);
void     gameobject_mark_as_renderable(random_uuid_t goUUID, bool value);

mat4s gameobject_get_transform(random_uuid_t goUUID);
mat4s gameobject_ptr_get_transform(GameObject* obj);
// TODO: Refactor to return values instead
void gameobject_get_position(random_uuid_t goUUID, vec3* position);
void gameobject_get_rotation(random_uuid_t goUUID, versor* rorationQuad);
void gameobject_get_rotation_euler(random_uuid_t goUUID, vec3* rotationEuler);
void gameobject_get_scale(random_uuid_t goUUID, float* scale);

// Transform Utils
void gameobject_set_transform(random_uuid_t goUUID, Transform transform);
void gameobject_set_position(random_uuid_t goUUID, vec3 position);
void gameobject_set_rotation(random_uuid_t goUUID, versor rorationQuad);
void gameobject_set_rotation_euler(random_uuid_t goUUID, vec3 rotationEuler);
void gameobject_set_scale(random_uuid_t goUUID, float scale);

#endif
