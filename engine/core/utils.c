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

void init_glfw(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

}

void init_glad(){
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

char *readFileToString(const char *filename) {
  // Open the file in read mode ("r")
  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("Could not open file %s\n", filename);
    fflush(stdout);
    return NULL;
  }

  // Move the file pointer to the end of the file to determine file size
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  rewind(file); // Move file pointer back to the beginning

  // Allocate memory for the file content (+1 for the null terminator)
  char *content = (char *)malloc((fileSize + 1) * sizeof(char));
  if (!content) {
    printf("Memory allocation failed\n");
    fclose(file);
    return NULL;
  }

  // Read file contents into the string
  fread(content, sizeof(char), fileSize, file);
  content[fileSize] = '\0'; // Null-terminate the string

  // Close the file and return the content
  fclose(file);
  return content;
}

