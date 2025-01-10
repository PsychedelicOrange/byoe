#include "player.h"
#include "camera_controller.h"

#include <logging/log.h>
#include <rng/rng.h>
#include <scene/sdf_scene.h>
#include <render/renderer_sdf.h>

#define TEST_SCENE 1

int game_main(void)
{
    REGISTER_GAME_OBJECT("Camera", 0, Camera_Start, Camera_Update);
    REGISTER_GAME_OBJECT("Player", PlayerData, Player_Start, Player_Update);

#if TEST_SCENE
    // Define Test Scene
    SDF_Scene scene;
    sdf_scene_init(&scene);

    SDF_Primitive sphere = {
        .type = SDF_PRIM_Sphere,
        .transform = {
            .position = {0.0f, 0.0f, 0.0f},
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {1.0f, 1.0f, 1.0f}
        },
        .radius = 0.5f
    };
    sdf_scene_add_primitive(&scene, sphere);

    renderer_sdf_set_scene(&scene);

#endif

    return EXIT_SUCCESS;
}
