#include "sdf_scene.h"

#include "../core/frustum.h"
#include "camera.h"

#include <glad/glad.h>
#include <cglm/cglm.h>

static uint32_t s_GPUSceneNodesUBO;

void sdf_scene_init(SDF_Scene* scene)
{
    scene->culled_nodes_count = 0;
    scene->current_node_head = 0;

    // create the UBO to upload scene nodes data
    glGenBuffers(1, &s_GPUSceneNodesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, s_GPUSceneNodesUBO);
    // Initialize with NULL data
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SDF_NodeGPUData), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    // Ex. Binding
    // unsigned int viewProjectionBindingIndex = glGetUniformBlockIndex(this->Program, "SDFScene");
    // glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_UniformBuffer, 0, 2 * sizeof(glm::mat4));

    // TODO: Create bounding spheres for all the init nodes in the scene here
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
        .type = SDF_NODE_PRIMITIVE,
        .primitive = primitive,
        .is_ref_node = false,
        .is_culled = false
    };

    scene->nodes[scene->current_node_head++] = node; 
    return scene->current_node_head;
}

int sdf_scene_add_primitive_ref(SDF_Scene* scene, SDF_Primitive primitive)
{
        SDF_Node node = {
        .type = SDF_NODE_PRIMITIVE,
        .primitive = primitive,
        .is_ref_node = true,
        .is_culled = false
    };

    scene->nodes[scene->current_node_head++] = node; 
    return scene->current_node_head;
}

int sdf_scene_add_operation(SDF_Scene* scene, SDF_Operation operation)
{
    SDF_Node node = {
        .type = SDF_NODE_OPERATION,
        .operation = operation,
        .is_ref_node = true,
        .is_culled = false
    };

    scene->nodes[scene->current_node_head++] = node; 
    return scene->current_node_head;
}

void sdf_scene_upload_scene_nodes_to_gpu(SDF_Scene* scene)
{
    glBindBuffer(GL_UNIFORM_BUFFER, s_GPUSceneNodesUBO);
    for(uint32_t i = 0; i < scene->current_node_head; ++i) {

        const SDF_Node node = scene->nodes[i];
        SDF_NodeGPUData data;
        data.nodeType = node.type;
        if (node.type == SDF_NODE_PRIMITIVE) {  
            data.primType = node.primitive.type;
            glm_vec4_copy((vec4){node.primitive.transform.position[0],
            node.primitive.transform.position[1],
            node.primitive.transform.position[2],
            node.primitive.radius}, data.params.raw);
        } else {
            data.op = node.operation.type;
            data.left = node.operation.left;
            data.right = node.operation.right;
        }

        glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(SDF_NodeGPUData), sizeof(SDF_NodeGPUData), &data);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}