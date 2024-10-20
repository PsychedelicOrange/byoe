#ifndef SHADER_H
#define SHADER_H

#include <cglm/cglm.h>
#include <cglm/struct.h>

unsigned int compile_shader(char* shaderCode, int shaderType);

unsigned int create_program(unsigned int vertexShader, unsigned int fragmentShader);

void setUniformMat4(unsigned int shaderProgram, mat4s matrix, char* location);

char* readFileToString(const char* filename);

#endif
