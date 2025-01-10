#include "camera_controller.h"
#include "player.h"

#include <logging/log.h>
#include <render/renderer_sdf.h>
#include <rng/rng.h>
#include <scene/sdf_scene.h>

#define TEST_SCENE 1

int game_main(void)
{
    REGISTER_GAME_OBJECT("Camera", 0, Camera_Start, Camera_Update);
    REGISTER_GAME_OBJECT("Player", PlayerData, Player_Start, Player_Update);

#if TEST_SCENE
    // Define Test Scene
    SDF_Scene* scene = malloc(sizeof(SDF_Scene));
    sdf_scene_init(scene);

#if 1
    SDF_Primitive sphere = {
        .type      = SDF_PRIM_Sphere,
        .transform = {
            .position = {{0.0f, 2.0f, 0.0f}},
            .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
            .scale    = {{0.5f, 0.0f, 0.0f}}}, // only x is used as radius of the sphere
            .material = {
                .diffuse = {0.5f, 0.3f, 0.7f, 1.0f}
            }
        };
    sdf_scene_add_primitive(scene, sphere);

    SDF_Primitive cube = {
        .type      = SDF_PRIM_Cube,
        .transform = {
            .position = {{2.0f, 0.0f, 0.0f}},
            .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
            .scale    = {{1.0f, 1.0f, 1.0f}}},
            .material = {
                .diffuse = {0.7f, 0.3f, 0.3f, 1.0f}
            }
        };
    sdf_scene_add_primitive(scene, cube);
#endif

    // Meta ball of 3 spheres
    SDF_Primitive sphere1 = {
    .type      = SDF_PRIM_Sphere,
    .transform = {
        .position = {{1.0f, 1.0f, 0.0f}},
        .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
        .scale    = {{1.5f, 0.0f, 0.0f}}}, // only x is used as radius of the sphere
        .material = {
            .diffuse = {0.5f, 0.7f, 0.3f, 1.0f}
        }
    };
    int prim1 = sdf_scene_add_primitive_ref(scene, sphere1);
    sphere1.transform.position = (vec3s){{1.0f, 0.0f, 0.0f}};
    int prim2 = sdf_scene_add_primitive_ref(scene, sphere1);

    SDF_Object obj = {
        .type = SDF_BLEND_SUBTRACTION,
        .prim_a = prim2,
        .prim_b = prim1
    };

    sdf_scene_add_object(scene, obj);

    // TODO: extend API to enable sdf_scene_add_object_ref to combine multiple primitives
    // sphere1.transform.position = (vec3s){0.0f, 0.0f, 0.0f};
    // int prim2 = sdf_scene_add_primitive_ref(scene, sphere1);
    // SDF_Object metaballObj = {
    //     .type = SDF_BLEND_UNION,
    //     .prim_a = metaball_int,
    //     .prim_b = 
    // }
    // int metaball_int = sdf_scene_add_object(scene, obj);

    renderer_sdf_set_scene(scene);

#endif

    return EXIT_SUCCESS;
}
