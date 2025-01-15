#include "sdf_scene.h"

#include "../core/frustum.h"
#include "../core/logging/log.h"
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

    // create the UBO to upload scene nodes data
    glGenBuffers(1, &s_GPUSceneNodesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, s_GPUSceneNodesUBO);
    // Initialize with NULL data
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SDF_NodeGPUData) * MAX_SDF_NODES, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
        .is_culled   = false};

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
        .is_culled   = false};

    // mark the hieararchy of it's nodes as ref_nodes
    scene->nodes[operation.prim_a].is_ref_node = true;
    scene->nodes[operation.prim_b].is_ref_node = true;

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
        const SDF_Node  node = scene->nodes[i];
        SDF_NodeGPUData data;
        memset(&data, 0, sizeof(SDF_NodeGPUData));

        data.nodeType    = node.type;
        data.is_ref_node = node.is_ref_node;
        if (node.type == SDF_NODE_PRIMITIVE) {
            data.primType = node.primitive.type;
            glm_vec4_copy((vec4) {node.primitive.transform.position.x,
                              node.primitive.transform.position.y,
                              node.primitive.transform.position.z,
                              node.primitive.transform.scale},
                data.pos_scale.raw);

            data.material = node.primitive.material;
        } else {
            data.op    = node.object.type;
            data.left  = node.object.prim_a;
            data.right = node.object.prim_b;

            glm_vec4_copy((vec4) {node.object.transform.position.x,
                              node.object.transform.position.y,
                              node.object.transform.position.z,
                              node.object.transform.scale},
                data.pos_scale.raw);
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
