//#include "cglm/struct/vec3.h"
#include "render/render_utils.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdio.h>
/*
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
*/
#include "logging/log.h"

// use this include which has all the neccessary flags
#include "add_primitive.h"
#include "file_browser.h"
#include "icon.h"
#include "menu.h"
#include "nuklear_include.h"
#include "sdf/sdf_format/sdf_file_types.h"
#include "sidebar.h"
#include "viewport.h"

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

/* Global UI elements */

menu_state                 menu;
sidebar_state              sidebar;
struct file_browser        browser;
struct add_primitive_state add_prim;
struct media               media;
struct viewport_state      viewport;
struct format_sdf_file     file;

struct format_sdf_file file_default()
{
    struct format_sdf_file def =
        {
            .version          = SDF_VERSION,
            .primitives       = malloc(sizeof(struct format_sdf_primitive)),
            .primitives_count = 1,
            .blends_count     = 0,
            .blends           = NULL};
    def.primitives[0].index                       = 0;
    def.primitives[0].label                       = "one ballz";
    def.primitives[0].type                        = SDF_PRIM_Sphere;
    def.primitives[0].props.sphere.radius         = 1;
    def.primitives[0].operations_count            = 1;
    def.primitives[0].operations                  = malloc(sizeof(format_sdf_operation));
    def.primitives[0].operations[0].type          = SDF_OP_TRANSLATION;
    def.primitives[0].operations[0].parameters[0] = 0;
    def.primitives[0].operations[0].parameters[1] = 0;
    def.primitives[0].operations[0].parameters[3] = 0;
    return def;
}

// Mouse update
struct mouse
{
    float lastX, lastY;
    float isRightMouseDown;
    float isLeftMouseDown;
    float isMiddleMouseDown;
    float scroll;
} mouse = {.lastX = WINDOW_WIDTH / 2., .lastY = WINDOW_HEIGHT / 2.};
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void) window;
    float xoffset = xpos - mouse.lastX;
    float yoffset = mouse.lastY - ypos;    // reversed since y-coordinates go from bottom to top
    mouse.lastX   = xpos;
    mouse.lastY   = ypos;

    xoffset *= 0.1;
    yoffset *= 0.1;

    update_camera_mouse_callback(&viewport, xoffset, yoffset);
}
// Keyboard shortcuts
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // open
    if (key == GLFW_KEY_O && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
        file_browser_free(&browser);
        browser.operation = browser_open;
        file_browser_init(&browser, &media);
    }
    // save as
    if (key == GLFW_KEY_S && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL) && (mods & GLFW_MOD_SHIFT)) {
        file_browser_free(&browser);
        browser.operation = browser_save;
        file_browser_init(&browser, &media);
    }
    // exit app
    if (key == GLFW_KEY_Q && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
        exit(1);
    }
    // toggle menu bar
    if (key == GLFW_KEY_LEFT_ALT && action == GLFW_RELEASE) {
        //keyPressed = 0;
        menu.show = !menu.show;
        if (menu.show) {
            sidebar.y      = WINDOW_HEIGHT * 0.03;
            sidebar.height = WINDOW_HEIGHT * 0.97;
        } else {
            sidebar.height = WINDOW_HEIGHT;
            sidebar.y      = 0;
        }
    }
    // toggle sidebar
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        sidebar.show = !sidebar.show;
    }
    // reload viewport
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        viewport = viewport_default(menu.window);
    }
    // editor shortcuts
    if (1) {    // add mouse pointer in editor condition
        if (key == GLFW_KEY_A && action == GLFW_PRESS) {
            add_prim.show = 1;
        }
    }
    // viewport controls
    if (key == GLFW_KEY_UP) {
        viewport.camera.position = glms_vec3_add(glms_vec3_scale_as(viewport.camera.front, 0.01), viewport.camera.position);
    }
}
void mouse_scroll_callback(GLFWwindow* win, double xoff, double yoff)
{
    nk_gflw3_scroll_callback(win, xoff, yoff);
    viewport_zoom(&viewport, xoff, yoff);
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    //int i;
    //for (i = 0;  i < count;  i++)
    //handle_dropped_file(paths[0]);
}
/*
static void error_callback(int e, const char* d)
{
    fprintf(stderr, "Error %d: %s\n", e, d);
}

*/

void init_glad()
{
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        printf("Failed to initialize glad");
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    nk_glfw3_mouse_button_callback(window, button, action, mods);
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        if (mods & GLFW_MOD_SHIFT) {
            viewport.pan_mode = 1;
        } else {
            viewport.orbit_mode = 1;
        }
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
        viewport.orbit_mode = 0;
        viewport.pan_mode   = 0;
    }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    viewport_window_resize_callback(&viewport, width, height);
    menu.window[0] = width;
    menu.window[1] = height;
    sidebar.height = height;
    sidebar.width  = width * .3;
}

GLFWwindow* create_glfw_window()
{
    render_utils_init_glfw();
    GLFWwindow* win = render_utils_create_glfw_window("sdf-editor", WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, key_callback);
    glfwSetDropCallback(win, drop_callback);
    glfwSetCursorPosCallback(win, cursor_position_callback);
    glfwSetWindowSizeCallback(win, window_size_callback);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetScrollCallback(win, mouse_scroll_callback);
    //    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    return win;
}

void load_fonts_cursor(struct nk_glfw* glfw, struct nk_context* ctx)
{
    int                   use_custom_font = 0;
    struct nk_font_atlas* atlas;
    nk_glfw3_font_stash_begin(glfw, &atlas);
    if (use_custom_font) {
        struct nk_font* clean = nk_font_atlas_add_from_file(atlas, "./resources/ProggyClean.ttf", 18, 0);
        nk_glfw3_font_stash_end(glfw);
        nk_style_set_font(ctx, &clean->handle);
    }
    nk_glfw3_font_stash_end(glfw);
    //nk_style_load_all_cursors(ctx, atlas->cursors);
}
void load_icons()
{
    glEnable(GL_TEXTURE_2D);
    media.icons.home         = icon_load("resources/home.png");
    media.icons.directory    = icon_load("resources/directory.png");
    media.icons.computer     = icon_load("resources/computer.png");
    media.icons.desktop      = icon_load("resources/desktop.png");
    media.icons.default_file = icon_load("resources/default.png");
    media.icons.text_file    = icon_load("resources/text.png");
    media.icons.music_file   = icon_load("resources/music.png");
    media.icons.font_file    = icon_load("resources/font.png");
    media.icons.img_file     = icon_load("resources/img.png");
    media.icons.movie_file   = icon_load("resources/movie.png");
    media_init(&media);
}

int main(void)
{
    /* boiler */
    struct nk_glfw     glfw = {0};
    GLFWwindow*        win  = create_glfw_window();
    struct nk_context* ctx;

    init_glad();
    /* OpenGL */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    ctx = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);
    load_fonts_cursor(&glfw, ctx);
    load_icons();

    int window[2];
    window[0] = WINDOW_WIDTH;
    window[1] = WINDOW_HEIGHT;
    glfwGetWindowSize(win, &window[0], &window[1]);
    struct nk_colorf bg;
    bg.r = 0;
    bg.b = 0;
    bg.g = 0;
    bg.a = 0;

    //editor layout state
    menu     = menu_default(window);
    sidebar  = sidebar_default(window);
    add_prim = add_primitive_default(window);
    viewport = viewport_default(window);
    file     = file_default();

    while (!glfwWindowShouldClose(win)) {
        /* Input */
        glfwPollEvents();

        /* Handle GUI state changes */
        switch (menu.current_action) {
            case menu_open:
                file_browser_free(&browser);
                browser.operation = browser_open;
                file_browser_init(&browser, &media);
                menu.current_action = 0;
                break;
            case menu_exit:
                glfwTerminate();
                exit(1);
                break;
            case menu_save_as:
                file_browser_free(&browser);
                browser.operation = browser_save;
                file_browser_init(&browser, &media);
                menu.current_action = 0;
                break;
            case menu_view_sidebar:
                sidebar.show        = !sidebar.show;
                menu.current_action = 0;
                break;
            default:
                menu.current_action = 0;
                break;
        }
        switch (browser.action) {
            case browser_open:
                printf("Selected in open: %s", browser.file);
                // deserialize file and open
                browser.action = 0;
                break;
            case browser_save:
                printf("Selected in save as: %s", browser.file);
                // serialize file and save
                browser.action = 0;
                break;
            default:
                browser.action = 0;
                break;
        }
        switch (add_prim.action) {
            case add_primitive_add:
                browser.action = 0;
                break;
            case add_primitive_no_action:
                break;
            default:
                browser.action = 0;
                break;
        }
        /* Determine viewport size */
        if (sidebar.show) {
            viewport.x = sidebar.width;
            viewport.w = window[0] - sidebar.width;
            viewport_window_resize_callback(&viewport, viewport.w, viewport.h);
        } else {
            viewport.x = 0;
            viewport.w = window[0];
            viewport_window_resize_callback(&viewport, viewport.w, viewport.h);
        }
        // Draw viewport
        viewport_draw(&viewport, file);

        //Draw GUI
        nk_glfw3_new_frame(&glfw);
        draw_add_primitive(ctx, &add_prim);
        sidebar_draw(ctx, &sidebar);
        menu_draw(ctx, &menu);
        file_browser_run(&browser, ctx);

        // Draw
        glfwGetWindowSize(win, &window[0], &window[1]);
        glViewport(0, 0, window[0], window[1]);
        /* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
         * with blending, scissor, face culling, depth test and viewport and
         * defaults everything back into a default state.
         * Make sure to either a.) save and restore or b.) reset your own state after
         * rendering the UI. */
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, 512 * 1024, 128 * 1024);
        glfwSwapBuffers(win);
    }
    nk_glfw3_shutdown(&glfw);
    glfwTerminate();
    return 0;
}
