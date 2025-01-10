#include <glad/glad.h>
#include <stdio.h>
#include <stddef.h>
#include "shader.h"
// -- -- -- -- -- -- Shader functions -- -- -- -- -- --- --
//

unsigned int create_shader_compute(char* computePath){
	unsigned int computeShader = compile_shader(computePath, GL_COMPUTE_SHADER);
	return create_program_compute(computeShader);
}
unsigned int create_shader(char* vertexPath,char* fragmentPath){
	unsigned int vertexShader = compile_shader(vertexPath, GL_VERTEX_SHADER);
	unsigned int fragShader = compile_shader(fragmentPath, GL_FRAGMENT_SHADER);
	return create_program(vertexShader, fragShader);
}
unsigned int compile_shader(char * filePath, int shaderType){
	char* shaderCode = readFileToString(filePath);
	unsigned int shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, (const GLchar**)&shaderCode, NULL);
	glCompileShader(shader);
	// check for shader compile errors
	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLint logLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

		char* infoLog = (char*)malloc(logLength * sizeof(char));
		glGetShaderInfoLog(shader, logLength, NULL, infoLog);

		fprintf(stderr, "\nFailed to compile shader: %s\n", infoLog);
		free(infoLog);
	}
	free(shaderCode);
	return shader;
}

unsigned int create_program_compute(unsigned int computeShader){
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, computeShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
	int success;
	char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		printf("\nfailed to link shaders: %s",infoLog);
		return (unsigned int) - 1;
    }
    glDeleteShader(computeShader);
	return shaderProgram;
}
unsigned int create_program(unsigned int vertexShader, unsigned int fragmentShader){
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
	int success;
	char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		printf("\nfailed to link shaders: %s",infoLog);
		return (unsigned int) - 1;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
	return shaderProgram;
}

void setUniformInt(unsigned int shaderProgram, int value, char* location){
	glUseProgram(shaderProgram);
	int loc = glGetUniformLocation(shaderProgram,location);
	glUniform1iv(loc, 1, &value);	
}

void setUniformVec2Int(unsigned int shaderProgram, int vector[2], char* location){
	glUseProgram(shaderProgram);
	int loc = glGetUniformLocation(shaderProgram,location);
	glUniform2iv(loc,1,vector);	
}
void setUniformMat4(unsigned int shaderProgram,mat4s matrix, char* location){
	glUseProgram(shaderProgram);
	int loc = glGetUniformLocation(shaderProgram,location);
	glUniformMatrix4fv(loc,1,GL_FALSE,&matrix.col[0].raw[0]);	
}

char* readFileToString(const char* filename) {
	// Open the file in read mode ("r")
	FILE* file = fopen(filename, "r");
	if (!file) {
		printf("\nCould not open file %s\n", filename);
		fflush(stdout);
		return NULL;
	}

	// Move the file pointer to the end of the file to determine file size
	fseek(file, 0, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file); // Move file pointer back to the beginning

	// Allocate memory for the file content (+1 for the null terminator)
	char* content = (char*)malloc((fileSize + 1) * sizeof(char));
	if (!content) {
		printf("\nMemory allocation failed\n");
		fclose(file);
		return NULL;
	}
	for (size_t i = 0; i < fileSize + 1; i++) {
		content[i] = '\0';
	}
	// Read file contents into the string
	fread(content, sizeof(char), fileSize, file);

	// Close the file and return the content
	fclose(file);
	return content;
}
