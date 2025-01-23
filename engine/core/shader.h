#ifndef SHADER_H
#define SHADER_H

#include <cglm/cglm.h>
#include <cglm/struct.h>

unsigned int create_shader_compute(char* computePath);
unsigned int create_shader(char* vertexPath, char* shaderPath);
unsigned int compile_shader(char* shaderCode, int shaderType);

unsigned int create_program_compute(unsigned int computeShader);
unsigned int create_program(unsigned int vertexShader, unsigned int fragmentShader);

void setUniformInt(unsigned int shaderProgram, int value, char* location);
void setUniformVec2Int(unsigned int shaderProgram, int vector[2], char* location);
void setUniformMat4(unsigned int shaderProgram, mat4s matrix, char* location);

char* readFileToString(const char* filename, uint32_t* file_size);

#endif
