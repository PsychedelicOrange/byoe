#include "sdf_scene.h"

#include "../core/frustum.h"
#include "../core/logging/log.h"
#include "../core/math_util.h"
#include "camera.h"

#include <cglm/cglm.h>
#include <glad/glad.h>

#include <string.h>    // memset

static uint32_t s_GPUSceneNodesUBO;

void sdf_scene_init(SDF_Scene* scene)
{
    scene->culled_nodes_count = 0;
    scene->current_node_head  = 0;
    scene->nodes              = calloc(MAX_SDF_NODES, sizeof(SDF_Node));
}

void sdf_scene_destroy(SDF_Scene* scene)
{
    glDeleteBuffers(1, &s_GPUSceneNodesUBO);
    free(scene);
    scene = NULL;
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

void sdf_scene_upload_scene_nodes_to_gpu(const SDF_Scene* scene)
{
    if (!scene)
        return;
}