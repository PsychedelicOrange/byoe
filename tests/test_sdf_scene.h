#include <stdio.h>
#include <string.h>

#include "test.h"

#include <engine/core/uuid/uuid.h>
#include <engine/engine.h>

#include <GLFW/glfw3.h>

static const float YAW     = -90.0f;
static const float PITCH   = 0.0f;
static vec3s       WorldUp = {{0, 1, 0}};

void setup_test_camera(void)
{
    GameState* gameState = gamestate_get_global_instance();
    Camera*    camera    = &gameState->camera;

    // Init some default values for the camera
    camera->position.x = 0;
    camera->position.y = 0;
    camera->position.z = 7;

    camera->front.x = 0;
    camera->front.y = 0;
    camera->front.z = 1;

    camera->up.x = WorldUp.x;
    camera->up.y = WorldUp.y;
    camera->up.z = WorldUp.z;

    camera->right.x = 1;
    camera->right.y = 0;
    camera->right.z = 0;

    camera->yaw   = YAW;
    camera->pitch = PITCH;

    camera->near_plane = 0.01f;
    camera->far_plane  = 100.0f;
    camera->fov        = 45.0f;

    // Calculate new front vector using yaw and pitch
    vec3s front;
    front.x       = cosf(glm_rad(camera->yaw)) * cosf(glm_rad(camera->pitch));
    front.y       = sinf(glm_rad(camera->pitch));
    front.z       = sinf(glm_rad(camera->yaw)) * cosf(glm_rad(camera->pitch));
    camera->front = glms_vec3_normalize(front);

    // Recalculate the camera's right vector after the front vector is updated
    camera->right = glms_vec3_normalize(glms_vec3_cross(camera->front, WorldUp));
    camera->up    = glms_vec3_normalize(glms_vec3_cross(camera->right, camera->front));    // Up vector (recalculated)

    // Update the camera's lookAt matrix
    glm_look(camera->position.raw, camera->front.raw, WorldUp.raw, camera->lookAt.raw);
}

void setup_sdf_test_scene(void)
{
    setup_test_camera();

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
            // .props.sphere = {.radius = 0.1f},
            .props.box = {.dimensions = {{1.0f, 1.0f, 1.0f}}},
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

        sphere1.transform.position = (vec3s){{demoStartX + 0.25f, 0.25f, 0.0f}};
        int prim2                  = sdf_scene_add_primitive(scene, sphere1);
        (void) prim2;

        sphere1.transform.position = (vec3s){{demoStartX + 0.5f, 0.0f, 0.0f}};
        int prim3                  = sdf_scene_add_primitive(scene, sphere1);
        (void) prim3;

        sphere1.transform.position = (vec3s){{demoStartX + 0.25f, -0.25f, 0.0f}};
        int prim4                  = sdf_scene_add_primitive(scene, sphere1);
        (void) prim4;

        SDF_Object meta_def = {
            .type   = SDF_BLEND_SMOOTH_UNION,
            .prim_a = sdf_scene_add_object(scene, (SDF_Object){.type = SDF_BLEND_SMOOTH_UNION, .prim_a = prim1, .prim_b = prim2}),
            .prim_b = sdf_scene_add_object(scene, (SDF_Object){.type = SDF_BLEND_SMOOTH_UNION, .prim_a = prim3, .prim_b = prim4})};

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
        sphere.transform.position   = (vec3s){{demoStartX, -0.125f, 0.0625f}};
        sphere.props.box.dimensions = (vec3s){{0.25f, 0.25f, 0.25f}};
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
        renderer_sdf_set_scene(scene);
    }
}

int game_main(void)
{
    setup_sdf_test_scene();

    return EXIT_SUCCESS;
}

static void write_texture_readback_to_ppm(const gfx_texture_readback* texture, const char* filename)
{
    if (!texture || !texture->pixels || texture->bits_per_pixel != 32) {
        LOG_ERROR("Invalid texture data or format");
        return;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        LOG_ERROR("Failed to open file: %s", filename);
        return;
    }

    // PPM header
    fprintf(file, "P6\n%u %u\n255\n", texture->width, texture->height);

    for (uint32_t y = 0; y < texture->height; ++y) {
        for (uint32_t x = 0; x < texture->width; ++x) {
            uint32_t index = (y * texture->width + x) * 4;
            // BGRA8_UNORM format
            uint8_t b = (uint8_t) ((texture->pixels[index + 0]));
            uint8_t g = (uint8_t) ((texture->pixels[index + 1]));
            uint8_t r = (uint8_t) ((texture->pixels[index + 2]));
            //uint8_t  a     = (uint8_t) ((texture->pixels[index + 3] / 255.0f) * 255);
            fwrite(&r, 1, 1, file);
            fwrite(&g, 1, 1, file);
            fwrite(&b, 1, 1, file);
        }
    }

    fclose(file);
    LOG_SUCCESS("Successfully wrote texture to %s", filename);
}

bool read_ppm(const char* filename, int* width, int* height, uint8_t** pixels)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        LOG_ERROR("Failed to open PPM file: %s\n", filename);
        return false;
    }

    // Read the PPM header (magic number, width, height, and max color value)
    char magic[3];
    if (fscanf(file, "%2s", magic) != 1 || magic[0] != 'P' || magic[1] != '6') {
        LOG_ERROR("Invalid PPM format in file: %s\n", filename);
        fclose(file);
        return false;
    }

    // Skip comments

    int ch;
    while ((ch = fgetc(file)) == '#') {
        while ((ch = fgetc(file)) != '\n' && ch != EOF)
            ;    // Skip to end of comment
    }

    if (fscanf(file, "%d %d", width, height) != 2) {
        LOG_ERROR("Failed to read image dimensions in file: %s\n", filename);
        fclose(file);
        return false;
    }

    int max_color_value;
    if (fscanf(file, "%d", &max_color_value) != 1 || max_color_value != 255) {
        LOG_ERROR("Invalid max color value in file: %s\n", filename);
        fclose(file);
        return false;
    }

    while ((ch = fgetc(file)) == '\n' || ch == ' ')
        ;

    size_t file_size = (*width) * (*height) * 3;
    *pixels          = (uint8_t*) malloc(file_size);
    if (!*pixels) {
        LOG_ERROR("Failed to allocate memory for pixel data\n");
        fclose(file);
        return false;
    }

    size_t read_count = fread(*pixels, 1, file_size, file);
    UNUSED(read_count);

    fclose(file);
    return true;
}

float compare_ppm_similarity(const char* file1, const char* file2)
{
    int      width1, height1, width2, height2;
    uint8_t* pixels1 = NULL;
    uint8_t* pixels2 = NULL;

    // Read the first PPM file
    if (!read_ppm(file1, &width1, &height1, &pixels1)) {
        return 0.0f;    // Indicates failure
    }

    // Read the second PPM file
    if (!read_ppm(file2, &width2, &height2, &pixels2)) {
        free(pixels1);
        return 0.0f;    // Indicates failure
    }

    // Compare dimensions
    if (width1 != width2 || height1 != height2) {
        free(pixels1);
        free(pixels2);
        return 0.0f;    // Images are not comparable
    }

    // Calculate the similarity metric
    int total_pixels    = width1 * height1 * 3;    // Total number of color components
    int matching_pixels = 0;

    for (int i = 0; i < total_pixels; i++) {
        if (pixels1[i] == pixels2[i]) {
            matching_pixels++;
        }
    }

    // Free allocated memory
    free(pixels1);
    free(pixels2);

    // Compute and return the similarity percentage
    return (((float) matching_pixels) / (float) total_pixels) * 100.0f;
}

// Test function
void test_sdf_scene(void)
{
    const char* test_case = "test_sdf_scene";

    // Test the rendering results of the scene
    {
        TEST_START();

        GLFWwindow* testGameWindow = NULL;
        engine_init(&testGameWindow, 800, 600);

        char windowTitle[250];
        sprintf(windowTitle, "[TEST] BYOE SDF test scene");
        glfwSetWindowTitle(testGameWindow, windowTitle);

        renderer_sdf_set_capture_swapchain_ready();

        renderer_sdf_render();

        const gfx_texture_readback* swapchain_readback = renderer_sdf_get_last_swapchain_readback();
        (void) swapchain_readback;

        write_texture_readback_to_ppm(swapchain_readback, "./tests/test_sdf_scene.ppm");

        engine_destroy();

        TEST_END();

        ASSERT_CON(compare_ppm_similarity("./tests/test_sdf_scene_golden_image.ppm", "./tests/test_sdf_scene.ppm") > 95.0f, test_case, "Screenshot testing SDF test scene + Engine Ignition/Shutdown flow test");
    }
}
