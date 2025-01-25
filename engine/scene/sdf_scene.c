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

    glBindBuffer(GL_UNIFORM_BUFFER, s_GPUSceneNodesUBO);
    for (uint32_t i = 0; i < scene->current_node_head; ++i) {
        SDF_Node node = scene->nodes[i];

        // if (!node.is_dirty)
        //     return;

        scene->nodes[i].is_dirty = false;

        SDF_NodeGPUData data;
        memset(&data, 0, sizeof(SDF_NodeGPUData));

        data.nodeType = node.type;
        if (node.type == SDF_NODE_PRIMITIVE) {
            data.primType = node.primitive.type;

            mat4s transform = create_transform_matrix(node.primitive.transform.position.raw, node.primitive.transform.rotation, (vec3) {1.0f, 1.0f, 1.0f});
            data.transform  = transform;
            data.scale      = node.primitive.transform.scale;

            glm_vec4_copy(node.primitive.props.packed_data[0].raw, data.packed_params[0].raw);
            glm_vec4_copy(node.primitive.props.packed_data[1].raw, data.packed_params[1].raw);

            data.material = node.primitive.material;
        } else {
            data.blend  = node.object.type;
            data.prim_a = node.object.prim_a;
            data.prim_b = node.object.prim_b;

            mat4s transform = create_transform_matrix(node.object.transform.position.raw, node.object.transform.rotation, (vec3) {1.0f, 1.0f, 1.0f});
            data.transform  = transform;
            data.scale      = node.object.transform.scale;
        }

        glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(SDF_NodeGPUData), sizeof(SDF_NodeGPUData), &data);
    }
}

void sdf_scene_bind_scene_nodes(uint32_t shaderProgramID)
{
    unsigned int bindingPoint = glGetUniformBlockIndex(shaderProgramID, "SDFScene");
    glUniformBlockBinding(shaderProgramID, 0, bindingPoint);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, s_GPUSceneNodesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, s_GPUSceneNodesUBO);
}