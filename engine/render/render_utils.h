#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include <stdint.h>

// Forward decleration
struct GLFWwindow;

//boilerplate
void render_utils_init_glfw(void);
void render_utils_init_glad(void);
void render_utils_crash_game(char* msg);

struct GLFWwindow* render_utils_create_glfw_window(const char* title, uint32_t width, uint32_t height);

//debug drawing
unsigned int render_utils_setup_debug_cube(void);
unsigned int render_utils_setup_screen_quad(void);

#endif