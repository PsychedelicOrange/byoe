#include "camera_controller.h"
#include "player.h"

#include <logging/log.h>
#include <render/renderer_sdf.h>
#include <rng/rng.h>
#include <scene/sdf_scene.h>

#define TEST_SCENE 1

/*
TODO: Basic Gameplay
- [ ] Spaceship prototype and make it scriptable for gameplay programmers --> red color thingy as per discussion
- [ ] Render Bullets (Object pooling) + Style: [SDF Bloom](https://www.shadertoy.com/view/7stGWj)
- [ ] [Optional] _Volumetrics:_ Smoke for when asteroids are destroyed: animate noise and render some FX using SDF, no need of a particle system.
- [ ] Ghosts[Optional]: if volumetrics are hard/time constrained, use a glass like material for ghosts with refraction and alpha. use this for ghosts: https://www.shadertoy.com/view/X32BRz
*/

int game_main(void)
{
    SDF_Scene* asteroids_game_scene = malloc(sizeof(SDF_Scene));
    sdf_scene_init(asteroids_game_scene);
    renderer_sdf_set_scene(asteroids_game_scene);

    SDF_Primitive sphere = {
        .type      = SDF_PRIM_Cube,
        .transform = {
            .position = {{0.0f, 0.0f, 0.0f}},
            .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
            .scale    = {{0.25f, 0.1f, 0.5f}}},
        .material = {.diffuse = {0.75f, 0.3f, 0.2f, 1.0f}}};
    int player_prim_idx = sdf_scene_add_primitive(asteroids_game_scene, sphere);

    REGISTER_GAME_OBJECT("Camera", 0, Camera_Start, Camera_Update);
    REGISTER_GAME_OBJECT_WITH_NODE_IDX("Player", PlayerData, Player_Start, Player_Update, player_prim_idx);

    return EXIT_SUCCESS;
}
