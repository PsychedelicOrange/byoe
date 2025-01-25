#ifndef SDF_SCENE_H
#define SDF_SCENE_H

#include "../render/render_structs.h"
#include "game_state.h"    // camera

// Docs: https://github.com/PsychedelicOrange/byoe/pull/10

#define MAX_SDF_NODES 1024    // same as max game objects
#define MAX_SDF_OPS   32      // Max no of SDF operations that can be done to combine complex shapes

// Wen need to flatten the SDF_Node to pass it to GPU, this structs helps with that
// aligned at 16 bytes | total = 160 bytes
typedef struct SDF_NodeGPUData
{
    int nodeType;
    int primType;
    int _pad[2];

    mat4s transform;

    float scale;
    float _pad2[3];

    vec4s packed_params[2];

    // SDF_Operation GPU View
    int blend;
    int prim_a;
    int prim_b;
    int _pad3;

    SDF_Material material;
} SDF_NodeGPUData;

typedef struct SDF_Scene
{
    // TODO: use batch compaction and use a single array to reduce memory footprint
    SDF_Node* nodes;
    // SDF_Node culled_node_refs[MAX_SDF_NODES]; // Private and only used by renderer_sdf
    uint32_t culled_nodes_count;
    uint32_t current_node_head;
} SDF_Scene;

//---------------------------------------------------------

// Initializes the scene params and internal info and buffers
void sdf_scene_init(SDF_Scene* scene);

// Destroys and frees all scene resources
void sdf_scene_destroy(SDF_Scene* scene);

// Culls the nodes against the camera view frustum on CPU and returns the no. of active nodes
int sdf_scene_cull_nodes(SDF_Scene* scene);

// Add a primitive to the scene and return its node index
int sdf_scene_add_primitive(SDF_Scene* scene, SDF_Primitive primitive);

// Add a composite operation to the scene and return its node index, can be used in chain rule fashion to create more complex SDFs
int sdf_scene_add_object(SDF_Scene* scene, SDF_Object object);

// Uploads the scene nodes to GPU by flattening them using SDF_NodeGPUData struct
void sdf_scene_upload_scene_nodes_to_gpu(const SDF_Scene* scene);

// Binds the buffer that contains the flattened nodes to the given shader
void sdf_scene_bind_scene_nodes(uint32_t shaderProgramID);

#endif