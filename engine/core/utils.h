#ifndef UTILS_H
#define UTILS_H
//boilerplate
void crash_game(char* msg);
void init_glfw(void);
void init_glad(void);

//debug drawing
unsigned int setup_debug_cube();
unsigned int setup_screen_quad();
#endif
