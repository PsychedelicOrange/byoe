#include <stdio.h>
#include <string.h>

#include "test.h"

#include <engine/core/uuid/uuid.h>
#include <engine/engine.h>

#include <GLFW/glfw3.h>

static const float YAW     = -90.0f;
static const float PITCH   = 0.0f;
static vec3s       WorldUp = {{0, 1, 0}};

void setup_test_camera()
{
    GameState* gameState = gamestate_get_global_instance();
    Camera*    camera    = &gameState->camera;

    // Init some default values for the camera
    camera->position.x = 0;
    camera->position.y = 0;
    camera->position.z = 5;

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

void setup_sdf_test_scene()
{
    setup_test_camera();

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
            .prim_a = cast_prim,
            .prim_b = cube_prim};
        int mold_idx = sdf_scene_add_object(scene, cube_mold);
        (void) mold_idx;
    }

    renderer_sdf_set_scene(scene);
}

int game_main()
{
    setup_sdf_test_scene();

    return EXIT_SUCCESS;
}

void write_texture_to_ppm(const char* filename, char* pixelData, uint32_t width, uint32_t height, uint32_t bpp)
{
    FILE* file = fopen(filename, "wb");
    if (!file) {
        LOG_ERROR("Failed to open file: %s\n", filename);
        return;
    }

    fprintf(file, "P6\n%d %d\n255\n", width, height);

    for (int y = height - 1; y >= 0; y--) {
        fwrite(pixelData + y * width * bpp, 1, width * bpp, file);
    }

    fclose(file);
    printf("Texture successfully written to %s\n", filename);
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

    *pixels = (uint8_t*) malloc(*width * *height * 3);
    if (!*pixels) {
        LOG_ERROR("Failed to allocate memory for pixel data\n");
        fclose(file);
        return false;
    }

    fread(*pixels, 1, *width * *height * 3, file);

    fclose(file);
    return true;
}

// Function to compare two PPM files
bool compare_ppm_files(const char* file1, const char* file2)
{
    int      width1, height1, width2, height2;
    uint8_t* pixels1 = NULL;
    uint8_t* pixels2 = NULL;

    // Read the first PPM file
    if (!read_ppm(file1, &width1, &height1, &pixels1)) {
        return false;
    }

    // Read the second PPM file
    if (!read_ppm(file2, &width2, &height2, &pixels2)) {
        free(pixels1);
        return false;
    }

    // Compare dimensions and max color value (we assume 255 for max color value)
    if (width1 != width2 || height1 != height2) {
        free(pixels1);
        free(pixels2);
        return false;
    }

    // Compare the pixel data
    bool is_equal = true;
    for (int i = 0; i < width1 * height1 * 3; i++) {
        if (pixels1[i] != pixels2[i]) {
            is_equal = false;
            break;
        }
    }

    // Free allocated memory
    free(pixels1);
    free(pixels2);

    return is_equal;
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

        texture_readback swapchain_readback = renderer_sdf_get_last_swapchain_readback();
        (void) swapchain_readback;

        write_texture_to_ppm("./tests/test_sdf_scene.ppm", swapchain_readback.pixels, swapchain_readback.width, swapchain_readback.height, swapchain_readback.bits_per_pixel);

        engine_destroy();

        TEST_END();

        ASSERT_CON(compare_ppm_files("./tests/test_sdf_scene_golden_image.ppm", "./tests/test_sdf_scene.ppm"), test_case, "Testing Engine Start/Render/Close and validating test scene");
    }
}
