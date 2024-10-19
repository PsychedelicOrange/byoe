#include "gameobject.h"

#include "logging/log.h"

static void create_transform_matrix(mat4s* dest, vec3 position, versor rotation, vec3 scale) {
    // Create individual transformation matrices
    mat4s translation, rotationMatrix, scaling;

    // Create translation matrix
    glm_translate_make(translation.raw, position);

    // Create rotation matrix from versor (quaternion)
    glm_mat4_identity(rotationMatrix.raw); // Start with identity matrix
    glm_quat_rotate(translation.raw, rotation, rotationMatrix.raw); // Apply rotation

    // Create scaling matrix
    glm_scale_make(scaling.raw, scale);

    // Combine transformations: scaling -> rotation -> translation
    glm_mat4_mul(translation.raw, rotationMatrix.raw, dest->raw); // Apply rotation to translation
    glm_mat4_mul(dest->raw, scaling.raw, dest->raw); // Apply scaling to the combined transformation
}

//-------------------------------------------------------
// Public API

void gameobject_mark_as_renderable(random_uuid_t goUUID, bool value)
{
    GameObject* go = get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        go->isRenderable = value;
    }
}

mat4s gameobject_get_transform(random_uuid_t goUUID) {
    GameObject* obj = get_gameobject_by_uuid(goUUID);
    if (!obj) {
        mat4s i;
        memset(i.raw, 0, sizeof(mat4s));
        return i;
    }

    Transform* transform = &obj->transform;

    mat4s result;
    create_transform_matrix(&result, transform->position, transform->rotation, transform->scale);

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

    mat4s result;
    create_transform_matrix(&result, transform->position, transform->rotation, transform->scale);

    return result;
}

void gameobject_get_position(random_uuid_t goUUID, vec3* position)
{
    GameObject* obj = get_gameobject_by_uuid(goUUID);
    if (obj) {
        glm_vec3_copy(obj->transform.position, *position);
    }
    else {
        glm_vec3_zero(*position);
    }
}

void gameobject_get_rotation(random_uuid_t goUUID, versor* rotationQuad)
{
    GameObject* obj = get_gameobject_by_uuid(goUUID);
    if (obj) {
        glm_quat_copy(obj->transform.rotation, *rotationQuad);
    }
    else {
        glm_quat_identity(*rotationQuad);
    }
}

void gameobject_get_rotation_euler(random_uuid_t goUUID, vec3* rotationEuler)
{
    GameObject* obj = get_gameobject_by_uuid(goUUID);
    if (obj) {
        glm_vec3_copy(obj->transform.rotation, *rotationEuler);
    }
    else {
        glm_vec3_zero(*rotationEuler);
    }
}

void gameobject_get_scale(random_uuid_t goUUID, vec3* scale)
{
    GameObject* obj = get_gameobject_by_uuid(goUUID);
    if (obj) {
        glm_vec3_copy(obj->transform.scale, *scale);
    }
    else {
        glm_vec3_zero(*scale);
    }
}

void gameobject_set_transform(random_uuid_t goUUID, Transform transform)
{
    GameObject* go = get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        go->transform = transform;
    }
}

void gameobject_set_position(random_uuid_t goUUID, vec3 position)
{
    GameObject* go = get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        glm_vec3_copy(position, go->transform.position);
    }
    else
        LOG_ERROR("Failed to update position for gameobject: %s\n", uuid_to_string(&goUUID));
}

void gameobject_set_rotation(random_uuid_t goUUID, versor rotationQuat)
{
    GameObject* go = get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        glm_quat_copy(rotationQuat, go->transform.rotation);
    }
    else
        LOG_ERROR("Failed to update rotation for gameobject %s\n", uuid_to_string(&goUUID));
}

void gameobject_set_rotation_euler(random_uuid_t goUUID, vec3 rotationEuler)
{
    GameObject* go = get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        glm_euler_xyz_quat(rotationEuler, go->transform.rotation);
    }
    else
        LOG_ERROR("Failed to update rotation for gameobject %s\n", uuid_to_string(&goUUID));
}

void gameobject_set_scale(random_uuid_t goUUID, vec3 scale)
{
    GameObject* go = get_gameobject_by_uuid(goUUID);
    if (go != NULL) {
        glm_vec3_copy(scale, go->transform.scale);
    }
    else
        LOG_ERROR("Failed to update scale for gameobject %s\n", uuid_to_string(&goUUID));
}