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

    float demoStartX = -2.0f;

    // simple sphere
    {
        SDF_Primitive sphere = {
            .type      = SDF_PRIM_Sphere,
            .transform = {
                .position = {{demoStartX, 0.0f, 0.0f}},
                .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
                .scale    = {{0.25f, 0.0f, 0.0f}}},    // only x is used as radius of the sphere
            .material = {.diffuse = {0.5f, 0.3f, 0.7f, 1.0f}}};
        sdf_scene_add_primitive(scene, sphere);
        demoStartX += 1.0f;
    }

    // simple cube
    {
        SDF_Primitive cube = {
            .type      = SDF_PRIM_Cube,
            .transform = {
                .position = {{demoStartX, 0.0f, 0.0f}},
                .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
                .scale    = {{0.25f, 0.25f, 0.25f}}},
            .material = {.diffuse = {0.7f, 0.3f, 0.3f, 1.0f}}};
        sdf_scene_add_primitive(scene, cube);
        demoStartX += 1.0f;
    }

    // Meta ball of 3 spheres
    {
        SDF_Primitive sphere1 = {
            .type      = SDF_PRIM_Sphere,
            .transform = {
                .position = {{demoStartX, 0.0f, 0.0f}},
                .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
                .scale    = {{0.25f, 0.0f, 0.0f}}},
            .material = {.diffuse = {0.5f, 0.7f, 0.3f, 1.0f}}};
        int prim1 = sdf_scene_add_primitive(scene, sphere1);
        (void) prim1;

        sphere1.transform.position = (vec3s){{demoStartX + 0.5f, 0.5f, 0.0f}};
        int prim2                  = sdf_scene_add_primitive(scene, sphere1);
        (void) prim2;

        sphere1.transform.position = (vec3s){{demoStartX + 1.0f, 0.0f, 0.0f}};
        int prim3                  = sdf_scene_add_primitive(scene, sphere1);
        (void) prim3;

        sphere1.transform.position = (vec3s){{demoStartX + 0.5f, -0.5f, 0.0f}};
        int prim4                  = sdf_scene_add_primitive(scene, sphere1);
        (void) prim4;

        SDF_Object meta_def = {
            .type   = SDF_BLEND_SMOOTH_UNION,
            .prim_a = sdf_scene_add_object(scene, (SDF_Object){.type = SDF_BLEND_SMOOTH_UNION, .prim_a = prim1, .prim_b = prim2}),
            .prim_b = sdf_scene_add_object(scene, (SDF_Object){.type = SDF_BLEND_SMOOTH_UNION, .prim_a = prim3, .prim_b = prim4})};

        sdf_scene_add_object(scene, meta_def);
        demoStartX += 2.0f;
    }

    // more complex object (creating a mold in a cube using a smooth union of a cube and sphere)
    {
        SDF_Primitive cube_prim_def = {
            .type      = SDF_PRIM_Cube,
            .transform = {
                .position = {{demoStartX, 0.0f, 0.0f}},
                .rotation = {0.0f, 0.0f, 0.0f, 1.0f},
                .scale    = {{0.5f, 0.5f, 0.25f}}},
            .material = {.diffuse = {0.8f, 0.81f, 0.83f, 1.0f}}};
        int cube_prim = sdf_scene_add_primitive(scene, cube_prim_def);
        (void) cube_prim;

        SDF_Primitive sphere = {
            .type      = SDF_PRIM_Sphere,
            .transform = {
                .position = {{demoStartX, 0.2f, 0.25f}},
                .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
                .scale    = {{0.1f, 0.0f, 0.0f}}},
            .material = {.diffuse = {0.25f, 0.47f, 0.34f, 1.0f}}};
        int prim1 = sdf_scene_add_primitive(scene, sphere);

        sphere.type               = SDF_PRIM_Cube;
        sphere.transform.position = (vec3s){{demoStartX, -0.2f, 0.25f}};
        sphere.transform.scale    = (vec3s){{0.1f, 0.1f, 0.1f}};
        int prim2                 = sdf_scene_add_primitive(scene, sphere);

        SDF_Object meta_cast = {
            .type   = SDF_BLEND_SMOOTH_UNION,
            .prim_a = prim1,
            .prim_b = prim2};

        int cast_prim = sdf_scene_add_object(scene, meta_cast);
        (void) cast_prim;

        SDF_Object cube_mold = {
            .type   = SDF_BLEND_SUBTRACTION,
            .prim_a = cube_prim,
            .prim_b = cast_prim};
        sdf_scene_add_object(scene, cube_mold);
    }

    renderer_sdf_set_scene(scene);

#endif

    return EXIT_SUCCESS;
}
