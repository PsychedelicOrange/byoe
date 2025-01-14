#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

/* use this include which has all the neccessary flags */
#include "nuklear_include.h"

#include "menu.h"
#include "sidebar.h"
#include "file_browser.h"

#include "logging/log.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080


/* Global UI elements */
menu_state menu;
sidebar_state sidebar;
struct file_browser browser;
struct media media;

/* Keyboard shortcuts */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // open
    if (key == GLFW_KEY_O && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL) ){
	file_browser_free(&browser);
	browser.operation = browser_open;
	file_browser_init(&browser, &media);
    }
    // save as
    if (key == GLFW_KEY_S && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL) && (mods & GLFW_MOD_SHIFT)){
	file_browser_free(&browser);
	browser.operation = browser_save;
	file_browser_init(&browser, &media);
    }
    // exit app
    if (key == GLFW_KEY_Q && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL) ){
	exit(1);
    }
    // exit ui
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
	if(browser.show) browser.show = 0;
	if(menu.show) menu.show = 0;
	if(sidebar.show) sidebar.show = 0;
    }
    // toggle menu bar
    if(key == GLFW_KEY_LEFT_ALT && action == GLFW_RELEASE){
	    //keyPressed=0;
	menu.show = !menu.show;
	if(menu.show){
	    sidebar.y = WINDOW_HEIGHT * 0.03;
	    sidebar.height = WINDOW_HEIGHT * 0.97;
	}else{
	    sidebar.height = WINDOW_HEIGHT;
	    sidebar.y = 0;
	}
    }
    // toggle sidebar
    if(key == GLFW_KEY_TAB && action == GLFW_PRESS ){
	sidebar.show = !sidebar.show;
    }

}
void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    /*int i;*/
    /*for (i = 0;  i < count;  i++)*/
        //handle_dropped_file(paths[0]);
}

static void error_callback(int e, const char *d)
{fprintf(stderr,"Error %d: %s\n", e, d);}

void init_glad(){
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOG_ERROR("Failed to initialize glad");
    }
} 

void window_size_callback(GLFWwindow* window, int width, int height)
{
    menu.window[0] = width;
    menu.window[1] = height;
    sidebar.height = height;
    sidebar.width = width * .3;
}

GLFWwindow* create_glfw_window(){
    /* GLFW */
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stdout, "[GFLW] failed to init!\n");
        exit(1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "sdf_editor", NULL, NULL);
    glfwSetKeyCallback(win, key_callback);
    glfwSetDropCallback(win, drop_callback);
    glfwSetWindowSizeCallback(win, window_size_callback);
    glfwMakeContextCurrent(win);
    return win;
}


void load_fonts_cursor(struct nk_glfw* glfw, struct nk_context* ctx)
{
    int use_custom_font = 1;
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(glfw, &atlas);
    if(use_custom_font){
	struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "./resources/ProggyClean.ttf", 18, 0);
	nk_glfw3_font_stash_end(glfw);
	nk_style_set_font(ctx, &clean->handle);
    }
    nk_glfw3_font_stash_end(glfw);
    //nk_style_load_all_cursors(ctx, atlas->cursors);
}
void load_icons(){
    glEnable(GL_TEXTURE_2D);
    media.icons.home = icon_load("resources/home.png");
    media.icons.directory = icon_load("resources/directory.png");
    media.icons.computer = icon_load("resources/computer.png");
    media.icons.desktop = icon_load("resources/desktop.png");
    media.icons.default_file = icon_load("resources/default.png");
    media.icons.text_file = icon_load("resources/text.png");
    media.icons.music_file = icon_load("resources/music.png");
    media.icons.font_file =  icon_load("resources/font.png");
    media.icons.img_file = icon_load("resources/img.png");
    media.icons.movie_file = icon_load("resources/movie.png");
    media_init(&media);
}
int main(void)
{
    /* boiler */
    struct nk_glfw glfw = {0};
    GLFWwindow *win = create_glfw_window();
    struct nk_context *ctx;

    init_glad();
    /* OpenGL */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    ctx = nk_glfw3_init(&glfw, win, NK_GLFW3_INSTALL_CALLBACKS);
    load_fonts_cursor(&glfw,ctx);
    load_icons();


    int window[2];
    window[0] = 1920; window[1] = 1080;
    glfwGetWindowSize(win, &window[0], &window[1]);
    struct nk_colorf bg; bg.r = 0; bg.b = 0; bg.g =0; bg.a=0;

    /*editor layout state*/
    menu = menu_default(window);
    sidebar = sidebar_default(window);

    while (!glfwWindowShouldClose(win))
    {
        /* Input */
        glfwPollEvents();

	/* Handle GUI state changes */
	switch (menu.current_action){
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
		sidebar.show = !sidebar.show;
		menu.current_action = 0;
		break;
	    default:
		menu.current_action = 0;
		break;
	}
	switch(browser.action){
	    case browser_open:
		printf("Selected in open: %s", browser.file);
		browser.action = 0;
		break;
	    case browser_save:
		printf("Selected in save as: %s", browser.file);
		browser.action = 0;
		break;
	    default:
		browser.action = 0;
		break;
	}

	/* Draw GUI */
        nk_glfw3_new_frame(&glfw);
	sidebar_draw(ctx,&sidebar);
	menu_draw(ctx,&menu);
	file_browser_run(&browser, ctx);

	/* Draw */
        glfwGetWindowSize(win, &window[0], &window[1]);
        glViewport(0, 0, window[0], window[1]);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(bg.r, bg.g, bg.b, bg.a);
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
