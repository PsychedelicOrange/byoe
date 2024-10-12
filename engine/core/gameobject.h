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

// Q. How to solve sending/updating transform, we don't have transform component and mesh data
// sol.1: we can manually link mesh to GameObject via registration or util functions in start itself, like game_object_set_mesh
// game_object_set_transform etc. we still need to pass it GameObject reference tho
// so we can replace the scripting functions to take the GameObject itself instead of limited data from this struct, kinda like
// emulating this pointer like behaviour. We can hardcode stuff like Transform as it makes sense similar to uuid
// or we already have the UUID of the scriptibg functions for easy access of the below gameobject public API

typedef struct Transform
{
    vec3    position;
    float _padding1;
    versor  rotation; // glm::quat [x, y, z, w]
    vec3    scale;
    float _padding2;
} Transform;

typedef struct GameObject
{
    uuid_t uuid;
    Transform transform;
    uuid_t meshID;
    bool isRenderable;
    const char* typeName;       // Type of the game object (class/type)
    void* gameState;            // Global game state such as Input, world info, scene data etc. 
    void* gameObjectData;       // Object-specific data that can be serialized/reflected
    StartFunction startFn;      // Start function for the object
    UpdateFunction updateFn;    // Update function for the object
    // TODO: Add collision callback functions here if needed
} GameObject;

// TODO
void gameobject_set_mesh(uuid_t goUUID, uuid_t meshUUID);
// TODO
void gameobject_set_material(uuid_t goUUID, uuid_t materialUUID);
// TODO
void gameobject_set_renderable(uuid_t goUUID, uuid_t meshUUID, uuid_t materialUUID);

// Transform Utils
// TODO
void gameobject_set_transform(uuid_t goUUID, Transform transform);
// TODO
void gameobject_set_position(uuid_t goUUID, vec3 position);
// TODO
void gameobject_set_rotation(uuid_t goUUID, versor rorationQuad);
// TODO
void gameobject_set_rotation_euler(uuid_t goUUID, vec3 rotationEuler);
// TODO
void gameobject_set_scale(uuid_t goUUID, vec3 scale);

#endif
