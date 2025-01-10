#ifndef SDF_SCENE_H
#define SDF_SCENE_H

#include "../render/render_structs.h"
#include "game_state.h"    // camera

#define MAX_SDF_NODES 1024    // same as max game objects
#define MAX_SDF_OPS   32      // Max no of SDF operations that can be done to combine complex shapes

typedef enum SDF_NodeType
{
    SDF_NODE_PRIMITIVE,
    SDF_NODE_OPERATION
} SDF_NodeType;

typedef struct bounding_sphere
{
    vec3  pos;
    float radius;
} bounding_sphere;

typedef struct SDF_Node
{
    SDF_NodeType type;
    union
    {
        SDF_Primitive primitive;
        SDF_Operation operation;
    };
    bounding_sphere bounds;
    bool            is_ref_node;
    bool            is_culled;
} SDF_Node;

// Wen need to flatten the SDF_Node to pass it to GPU, this structs helps with that
typedef struct SDF_NodeGPUData
{
    int nodeType;

    // SDF_Primitive GPU View
    int   primType;
    int _pad[2];
    vec4s pos_scale;    // simplified view of params for the GPU

    // SDF_Operation GPU View
    int op;
    int left;     // Index of the left child node
    int right;    // Index of the right child node
    int _pad1;
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

// This is used to store primitive references in the SDF_scene to be later combined with operations
int sdf_scene_add_primitive_ref(SDF_Scene* scene, SDF_Primitive primitive);

// Add a composite operation to the scene and return its node index, can be used in chain rule fashion to create more complex SDFs
int sdf_scene_add_operation(SDF_Scene* scene, SDF_Operation operation);

void sdf_scene_upload_scene_nodes_to_gpu(const SDF_Scene* scene);

void sdf_scene_bind_scene_nodes(uint32_t shaderProgramID);

#endif