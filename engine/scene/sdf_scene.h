#ifndef SDF_SCENE_H
#define SDF_SCENE_H

#include "game_state.h" // camera

#include "../render/render_structs.h"

#define MAX_SDF_NODES 1024 // same ax mac game objects

typedef enum SDF_NodeType {
    SDF_NODE_PRIMITIVE,
    SDF_NODE_OPERATION
} SDF_NodeType;

typedef struct SDF_Node {
    SDF_NodeType type;
    union {
        SDF_Primitive primitive;
        SDF_Operation operation;
    };
} SDF_Node;

typedef struct SDF_Scene {
    // TODO: use batch compaction and use a single array to reduce memory footprint
    SDF_Node nodes[MAX_SDF_NODES];
    SDF_Node culled_nodes[MAX_SDF_NODES];
    int node_count; // Number of active nodes after culling
} SDF_Scene;

void sdf_scene_init(SDF_Scene* scene);
void sdf_scene_destroy(SDF_Scene* scene);

// Culls the nodes agains the camera view frustum on CPU and returns the no. of active nodes
int sdf_scene_cull_nodes(const SDF_Scene* scene, Camera camera);

// Add a primitive to the scene and return its node index
int sdf_scene_add_primitive(SDF_Scene* scene, SDF_Primitive primitive);

// Add a composite operation to the scene and return its node index
int sdf_scene_add_operation(SDF_Scene* scene, SDF_Operation operation);



#endif