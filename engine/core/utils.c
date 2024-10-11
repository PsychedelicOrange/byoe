#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "utils.h"

void crash_game(char* msg){
	printf("\nGame crashed: %s\n",msg);
	fflush(stdout);
	exit(1);
}

void init_glfw(void){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

}

void init_glad(void){
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
		crash_game("failed to initialize glad");
    }
	const GLubyte* vendor = glGetString(GL_VENDOR); // Returns the vendor
	const GLubyte* renderer = glGetString(GL_RENDERER); // Returns a hint to the model
	printf("\nVendor: %s",vendor);
	printf("\nRenderer: %s",renderer);
	fflush(stdout);
}


