#include "sdf_scene.h"

#include "../core/frustum.h"
#include "../core/logging/log.h"
#include "camera.h"

#include <cglm/cglm.h>
#include <glad/glad.h>

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
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SDF_NodeGPUData), NULL, GL_DYNAMIC_DRAW);
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

int sdf_scene_add_primitive_ref(SDF_Scene* scene, SDF_Primitive primitive)
{
    SDF_Node node = {
        .type        = SDF_NODE_PRIMITIVE,
        .primitive   = primitive,
        .is_ref_node = true,
        .is_culled   = false};

    uint32_t idx      = scene->current_node_head++;
    scene->nodes[idx] = node;
    return idx;
}

int sdf_scene_add_operation(SDF_Scene* scene, SDF_Operation operation)
{
    SDF_Node node = {
        .type        = SDF_NODE_OPERATION,
        .operation   = operation,
        .is_ref_node = true,
        .is_culled   = false};

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
        data.nodeType = node.type;
        if (node.type == SDF_NODE_PRIMITIVE) {
            data.primType = node.primitive.type;
            glm_vec4_copy((vec4){node.primitive.transform.position[0],
                              node.primitive.transform.position[1],
                              node.primitive.transform.position[2],
                              node.primitive.transform.scale[0]
                              },
                data.pos_scale.raw);
                LOG_INFO("vec4:(%f, %f, %f, %f)", data.pos_scale.x, data.pos_scale.y, data.pos_scale.z, data.pos_scale.w);
        } else {
            data.op    = node.operation.type;
            data.left  = node.operation.left;
            data.right = node.operation.right;
        }

        glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(SDF_NodeGPUData), sizeof(SDF_NodeGPUData), &data);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void sdf_scene_bind_scene_nodes(uint32_t shaderProgramID)
{
    unsigned int bindingPoint = glGetUniformBlockIndex(shaderProgramID, "SDFScene");
    glUniformBlockBinding(shaderProgramID, 0, bindingPoint);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, s_GPUSceneNodesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, s_GPUSceneNodesUBO);
}
