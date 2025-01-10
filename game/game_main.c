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

    SDF_Primitive sphere = {
        .type      = SDF_PRIM_Sphere,
        .transform = {
            .position = {0.0f, 0.0f, 0.0f},
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale    = {1.0f, 1.0f, 1.0f}},
        .radius = 0.85f};
    int nodeIdx = sdf_scene_add_primitive(scene, sphere);
    LOG_INFO("Sphere node idx: %d", nodeIdx);

    renderer_sdf_set_scene(scene);

#endif

    return EXIT_SUCCESS;
}
