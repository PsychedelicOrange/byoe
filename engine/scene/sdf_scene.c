#include "sdf_scene.h"

#include "../core/frustum.h"
#include "../core/logging/log.h"
#include "../core/math_util.h"
#include "camera.h"

#include <cglm/cglm.h>
#include <glad/glad.h>

#include <string.h>    // memset

static SDF_NodeGPUData* s_SceneGPUData = NULL;

void sdf_scene_init(SDF_Scene* scene)
{
    scene->culled_nodes_count = 0;
    scene->current_node_head  = 0;
    scene->nodes              = calloc(MAX_SDF_NODES, sizeof(SDF_Node));           // 176 bytes x 512 MiB
    s_SceneGPUData            = calloc(MAX_SDF_NODES, sizeof(SDF_NodeGPUData));    // 160 bytes x 512 MiB
}

void sdf_scene_destroy(SDF_Scene* scene)
{
    SAFE_FREE(s_SceneGPUData);
    SAFE_FREE(scene->nodes);
    SAFE_FREE(scene);
}

int sdf_scene_cull_nodes(SDF_Scene* scene)
{
    const Camera camera = gamestate_get_global_instance()->camera;
    (void) camera;
    (void) scene;
    // TODO: Check against camera frustum and node bounding sphere

    return scene->culled_nodes_count;
}

int sdf_scene_add_primitive(SDF_Scene* scene, SDF_Primitive primitive)
{
    SDF_Node node = {
        .type        = SDF_NODE_PRIMITIVE,
        .primitive   = primitive,
        .is_ref_node = false,
        .is_culled   = false,
        .is_dirty    = true};

    uint32_t idx      = scene->current_node_head++;
    scene->nodes[idx] = node;
    return idx;
}

int sdf_scene_add_object(SDF_Scene* scene, SDF_Object operation)
{
    SDF_Node node = {
        .type        = SDF_NODE_OBJECT,
        .object      = operation,
        .is_ref_node = false,
        .is_culled   = false,
        .is_dirty    = true};

    // mark the hierarchy of it's nodes as ref_nodes
    scene->nodes[operation.prim_a].is_ref_node = true;
    scene->nodes[operation.prim_b].is_ref_node = true;

    scene->nodes[operation.prim_a].is_dirty = true;
    scene->nodes[operation.prim_b].is_dirty = true;

    uint32_t idx      = scene->current_node_head++;
    scene->nodes[idx] = node;
    return idx;
}

void sdf_scene_update_scene_node_gpu_data(const SDF_Scene* scene)
{
    if (!scene)
        return;

    for (uint32_t i = 0; i < scene->current_node_head; i++) {
        SDF_NodeGPUData* gpuNode = &s_SceneGPUData[i];
        SDF_Node         node    = scene->nodes[i];

        gpuNode->nodeType = node.type;

        if (node.type == SDF_NODE_PRIMITIVE) {
            gpuNode->primType = node.primitive.type;
            gpuNode->material = node.primitive.material;

            mat4s transform    = create_transform_matrix(node.primitive.transform.position.raw, node.primitive.transform.rotation, (vec3){1.0f, 1.0f, 1.0f});
            gpuNode->transform = transform;
            gpuNode->scale     = node.primitive.transform.scale;

            glm_vec4_copy(node.primitive.props.packed_data[0].raw, gpuNode->packed_params[0].raw);
            glm_vec4_copy(node.primitive.props.packed_data[1].raw, gpuNode->packed_params[1].raw);

        } else {
            gpuNode->blend  = node.object.type;
            gpuNode->prim_a = node.object.prim_a;
            gpuNode->prim_b = node.object.prim_b;

            mat4s transform    = create_transform_matrix(node.object.transform.position.raw, node.object.transform.rotation, (vec3){1.0f, 1.0f, 1.0f});
            gpuNode->transform = transform;
            gpuNode->scale     = node.primitive.transform.scale;
        }
    }
}

void* sdf_scene_get_scene_nodes_gpu_data(const SDF_Scene* scene)
{
    if (!scene)
        return NULL;

    // parse thought scene and pack the SDF_NodeGPUData into a large buffer and return it
    return (void*) s_SceneGPUData;
}
