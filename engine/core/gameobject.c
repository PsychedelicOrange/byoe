#include "gameobject.h"

#include "logging/log.h"

#include "math_util.h"

//-------------------------------------------------------
// Public API

void gameobject_set_sdf_node_idx(random_uuid_t goUUID, uint32_t index)
{
    GameObject* go = game_registry_get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        go->sdfNodeIdx   = index;
        go->isRenderable = true;
    }
}
uint32_t gameobject_get_sdf_node_idx(random_uuid_t goUUID)
{
    GameObject* go = game_registry_get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        return go->sdfNodeIdx;
    }
    return UINT32_MAX;
}

void gameobject_mark_as_renderable(random_uuid_t goUUID, bool value)
{
    GameObject* go = game_registry_get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        go->isRenderable = value;
    }
}

mat4s gameobject_get_transform(random_uuid_t goUUID)
{
    GameObject* obj = game_registry_get_gameobject_by_uuid(goUUID);
    if (!obj) {
        mat4s i;
        memset(i.raw, 0, sizeof(mat4s));
        return i;
    }

    Transform* transform = &obj->transform;

    mat4s result = create_transform_matrix(transform->position.raw, transform->rotation, (vec3){transform->scale, transform->scale, transform->scale});

    return result;
}

mat4s gameobject_ptr_get_transform(GameObject* obj)
{
    if (!obj) {
        mat4s i;
        memset(i.raw, 0, sizeof(mat4s));
        return i;
    }

    Transform* transform = &obj->transform;

    mat4s result = create_transform_matrix(transform->position.raw, transform->rotation, (vec3){transform->scale, transform->scale, transform->scale});

    return result;
}

void gameobject_get_position(random_uuid_t goUUID, vec3* position)
{
    GameObject* obj = game_registry_get_gameobject_by_uuid(goUUID);
    if (obj) {
        glm_vec3_copy(obj->transform.position.raw, *position);
    } else {
        glm_vec3_zero(*position);
    }
}

void gameobject_get_rotation(random_uuid_t goUUID, versor* rotationQuad)
{
    GameObject* obj = game_registry_get_gameobject_by_uuid(goUUID);
    if (obj) {
        glm_quat_copy(obj->transform.rotation, *rotationQuad);
    } else {
        glm_quat_identity(*rotationQuad);
    }
}

void gameobject_get_rotation_euler(random_uuid_t goUUID, vec3* rotationEuler)
{
    GameObject* obj = game_registry_get_gameobject_by_uuid(goUUID);
    if (obj) {
        glm_vec3_copy(obj->transform.rotation, *rotationEuler);
    } else {
        glm_vec3_zero(*rotationEuler);
    }
}

void gameobject_get_scale(random_uuid_t goUUID, float* scale)
{
    GameObject* obj = game_registry_get_gameobject_by_uuid(goUUID);
    if (obj) {
        *scale = obj->transform.scale;
    } else {
        *scale = 1.0f;
    }
}

void gameobject_set_transform(random_uuid_t goUUID, Transform transform)
{
    GameObject* go = game_registry_get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        go->transform = transform;
    }
}

void gameobject_set_position(random_uuid_t goUUID, vec3 position)
{
    GameObject* go = game_registry_get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        glm_vec3_copy(position, go->transform.position.raw);
    } else
        LOG_ERROR("Failed to update position for gameobject: %s", uuid_to_string(&goUUID));
}

void gameobject_set_rotation(random_uuid_t goUUID, versor rotationQuat)
{
    GameObject* go = game_registry_get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        glm_quat_copy(rotationQuat, go->transform.rotation);
    } else
        LOG_ERROR("Failed to update rotation for gameobject %s", uuid_to_string(&goUUID));
}

void gameobject_set_rotation_euler(random_uuid_t goUUID, vec3 rotationEuler)
{
    GameObject* go = game_registry_get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        glm_euler_xyz_quat(rotationEuler, go->transform.rotation);
    } else
        LOG_ERROR("Failed to update rotation for gameobject %s", uuid_to_string(&goUUID));
}

void gameobject_set_scale(random_uuid_t goUUID, float scale)
{
    GameObject* go = game_registry_get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        go->transform.scale = scale;
    } else
        LOG_ERROR("Failed to update scale for gameobject %s", uuid_to_string(&goUUID));
}