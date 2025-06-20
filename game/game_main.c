#include "camera_controller.h"
#include "player.h"

#include <logging/log.h>
#include <render/renderer_sdf.h>
#include <rng/rng.h>
#include <scene/sdf_scene.h>

#define TEST_SCENE 1

int game_main(void)
{
#if !TEST_SCENE
    SDF_Scene* asteroids_game_scene = malloc(sizeof(SDF_Scene));
    sdf_scene_init(asteroids_game_scene);
    renderer_sdf_set_scene(asteroids_game_scene);

    versor init_quat;
    glm_euler_xyz_quat((vec3) {0.0f, glm_rad(45.0f), 0.0f}, init_quat);

    SDF_Primitive sphere = {
        .type      = SDF_PRIM_Box,
        .transform = {
            .position = {{0.0f, 0.0f, 0.0f}},
            //.rotation = {0.0f, 0.383f, 0.0f, 0.924f},
            .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
            .scale    = 1.0f},
        .props.box = {.dimensions = {{0.45f, 0.20f, 1.0f}}},
        .material  = {.diffuse = {0.75f, 0.3f, 0.2f, 1.0f}}};
    int player_prim_idx = sdf_scene_add_primitive(asteroids_game_scene, sphere);

    REGISTER_GAME_OBJECT(0, Camera_Start, Camera_Update);
    REGISTER_GAME_OBJECT_WITH_NODE_IDX(PlayerData, Player_Start, Player_Update, player_prim_idx);
#else
    REGISTER_GAME_OBJECT(0, Camera_Start, Camera_Update);

    // Define Test Scene
    SDF_Scene* scene = malloc(sizeof(SDF_Scene));
    sdf_scene_init(scene);

    float demoStartX = -0.75f;
    (void) demoStartX;

    // Testing All Primitives here
    {
        SDF_Primitive primitive_def = {
            .type      = SDF_PRIM_Sphere,
            .transform = {
                .position = {{demoStartX, 0.0f, 0.0f}},
                .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
                .scale    = 1.0f},
            .props.sphere = {.radius = 1.0f},
            // .props.box    = {.dimensions = {{1.0f, 1.0f, 1.0f}}},
            //.props.round_box        = {.dimensions = {1.0f, 1.0f, 1.0f}, .roundness = 0.5f},                      // TEST
            //.props.box_frame        = {.dimensions = {1.0f, 1.0f, 1.0f}, .thickness = 0.025f},                    // TEST
            //.props.torus            = {.thickness = {1.0f, 0.25f}},                                               // TEST
            //.props.torus_capped     = {.thickness = {1.0f, 0.25f}, .radiusA = 1.0f, .radiusB = 0.5f},             // TEST
            //.props.capsule          = {.start = {0.0f, 0.0f, 0.0f}, .end = {1.0f, 1.0f, 1.0f}, .radius = 1.0f},   // TEST
            //.props.vertical_capsule = {.radius = 0.25f, .height = 1.0f},                                          // TEST
            //.props.cylinder         = {.radius = 0.25f, .height = 1.0f},                                          // TEST
            //.props.rounded_cylinder = {.radiusA = 0.15f, .radiusB = 0.75f, .height = 1.0f},                       // TEST
            //.props.ellipsoid        = {.radii = {1.0f, 2.0f, 1.0f}},                                              // TEST
            //.props.hexagonal_prism  = {.height = 1.0f, .radius = 1.0f},                                           // TEST
            //.props.triangular_prism = {.height = 1.0f, .radius = 1.0f},                                           // TEST
            //.props.cone             = {.height = 1.0f, .angle = 0.78f},                                           // TEST
            //.props.capped_cone      = {.radiusTop = 0.25, .radiusBottom = 0.5f, .height = 1.0f},                  // TEST
            //.props.plane            = {.distance = 0.2f, .normal = {0.0f, 1.0f, 0.0f}},                           // TEST
            .material = {.diffuse = {0.5f, 0.3f, 0.7f, 1.0f}}};
        int prim = sdf_scene_add_primitive(scene, primitive_def);
        (void) prim;
        demoStartX += 0.5f;
    }

    // simple cube
    {
        SDF_Primitive cube = {
            .type      = SDF_PRIM_Box,
            .transform = {
                .position = {{demoStartX, 0.0f, 0.0f}},
                .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
                .scale    = 1.0f},
            .props.box = {.dimensions = {{0.5f, 0.25f, 0.5f}}},
            .material  = {.diffuse = {0.7f, 0.3f, 0.3f, 1.0f}}};
        sdf_scene_add_primitive(scene, cube);
        demoStartX += 0.5f;
    }

    // Meta ball of 3 spheres
    {
        SDF_Primitive sphere1 = {
            .type      = SDF_PRIM_Sphere,
            .transform = {
                .position = {{demoStartX, 0.0f, 0.0f}},
                .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
                .scale    = 1.0f},
            .props.sphere = {.radius = 0.25f},
            .material     = {.diffuse = {0.5f, 0.7f, 0.3f, 1.0f}}};
        int prim1 = sdf_scene_add_primitive(scene, sphere1);
        (void) prim1;

        sphere1.transform.position = (vec3s) {{demoStartX + 0.25f, 0.25f, 0.0f}};
        int prim2                  = sdf_scene_add_primitive(scene, sphere1);
        (void) prim2;

        sphere1.transform.position = (vec3s) {{demoStartX + 0.5f, 0.0f, 0.0f}};
        int prim3                  = sdf_scene_add_primitive(scene, sphere1);
        (void) prim3;

        sphere1.transform.position = (vec3s) {{demoStartX + 0.25f, -0.25f, 0.0f}};
        int prim4                  = sdf_scene_add_primitive(scene, sphere1);
        (void) prim4;

        SDF_Object meta_def = {
            .type   = SDF_BLEND_SMOOTH_UNION,
            .prim_a = sdf_scene_add_object(scene, (SDF_Object) {.type = SDF_BLEND_SMOOTH_UNION, .prim_a = prim1, .prim_b = prim2}),
            .prim_b = sdf_scene_add_object(scene, (SDF_Object) {.type = SDF_BLEND_SMOOTH_UNION, .prim_a = prim3, .prim_b = prim4})};

        sdf_scene_add_object(scene, meta_def);
        demoStartX += 0.65f;
    }

    // more complex object (creating a mold in a cube using a smooth union of a cube and sphere)
    {
        SDF_Primitive cube_prim_def = {
            .type      = SDF_PRIM_Box,
            .transform = {
                .position = {{demoStartX, 0.0f, -0.0f}},
                .rotation = {0.0f, 0.0f, 0.0f, 1.0f},
                .scale    = 1.0f},
            .props.box = {.dimensions = {{1.0f, 1.0f, 0.25f}}},
            .material  = {.diffuse = {0.8f, 0.81f, 0.83f, 1.0f}}};
        int cube_prim = sdf_scene_add_primitive(scene, cube_prim_def);
        (void) cube_prim;
        SDF_Primitive sphere = {
            .type      = SDF_PRIM_Sphere,
            .transform = {
                .position = {{demoStartX, 0.125f, 0.0625f}},
                .rotation = {0.0f, 0.0f, 0.0f, 0.0f},
                .scale    = 1.0f},
            .props.sphere = {.radius = 0.25f},
            .material     = {.diffuse = {0.25f, 0.47f, 0.34f, 1.0f}}};
        int prim1 = sdf_scene_add_primitive(scene, sphere);
        (void) prim1;

        sphere.type                 = SDF_PRIM_Box;
        sphere.transform.position   = (vec3s) {{demoStartX, -0.125f, 0.0625f}};
        sphere.props.box.dimensions = (vec3s) {{0.25f, 0.25f, 0.25f}};
        int prim2                   = sdf_scene_add_primitive(scene, sphere);
        (void) prim2;

        SDF_Object meta_cast = {
            .type      = SDF_BLEND_SMOOTH_UNION,
            .transform = {.position = {{demoStartX, 0.0f, 0.0f}}},
            .prim_a    = prim1,
            .prim_b    = prim2};

        int cast_prim = sdf_scene_add_object(scene, meta_cast);
        (void) cast_prim;

        SDF_Object cube_mold = {
            .type      = SDF_BLEND_SUBTRACTION,
            .transform = {.position = {{demoStartX, 0.0f, 0.0f}}},
            .prim_a    = cast_prim,
            .prim_b    = cube_prim};
        int mold_idx = sdf_scene_add_object(scene, cube_mold);
        (void) mold_idx;
    }

    renderer_sdf_set_scene(scene);
#endif    // TEST_SCENE

    return EXIT_SUCCESS;
}
