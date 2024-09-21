#include "glad/glad.h"
#include <math.h>
#include <stdlib.h>
#include "shapes.h"

GLuint setup_debug_cube(){
	    GLfloat vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,  // Bottom-left
         0.5f, -0.5f,  0.5f,  // Bottom-right
         0.5f,  0.5f,  0.5f,  // Top-right
        -0.5f,  0.5f,  0.5f,  // Top-left

        // Back face
        -0.5f, -0.5f, -0.5f,  // Bottom-left
         0.5f, -0.5f, -0.5f,  // Bottom-right
         0.5f,  0.5f, -0.5f,  // Top-right
        -0.5f,  0.5f, -0.5f   // Top-left
    };

    // Define the indices for the cube's faces
    GLuint indices[] = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        0, 4, 7, 7, 3, 0,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Top face
        3, 7, 6, 6, 2, 3,
        // Bottom face
        0, 4, 5, 5, 1, 0
    };

	GLuint VAO,VBO,EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind VAO
    glBindVertexArray(VAO);

    // Bind and set VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Bind and set EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind VAO (NOT EBO) to prevent accidental modification
    glBindVertexArray(0);
	return VAO;
}

Sphere generateSphere(float radius, int subdivisions) {
    // Generate vertex array object
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Generate vertex buffer object
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Calculate number of vertices and indices
    int numVertices = (subdivisions + 1) * (subdivisions + 1);
    int numIndices = 6 * subdivisions * subdivisions;

    // Allocate memory for vertices and indices
    GLfloat* vertices = malloc (sizeof(GLfloat) * numVertices * 3);
    GLuint* indices = malloc( sizeof( GLuint)* numIndices);

    // Generate sphere vertices and indices
	for (int i = 0; i <= subdivisions; ++i) {
		for (int j = 0; j <= subdivisions; ++j) {
			float theta = i * M_PI / subdivisions;
			float phi = j * 2 * M_PI / subdivisions;

			float x = radius * sin(theta) * cos(phi);
			float y = radius * sin(theta) * sin(phi);
			float z = radius * cos(theta);

			vertices[3 * (i * (subdivisions + 1) + j)] = x;
			vertices[3 * (i * (subdivisions + 1) + j) + 1] = y;
			vertices[3 * (i * (subdivisions + 1) + j) + 2] = z;

			if (i < subdivisions && j < subdivisions) {
				int topLeft = i * (subdivisions + 1) + j;
				int topRight = topLeft + 1;
				int bottomLeft = topLeft + subdivisions + 1;
				int bottomRight = bottomLeft + 1;  


				indices[6 * (i * subdivisions + j)] = topLeft;
				indices[6 * (i * subdivisions + j) + 1] = bottomRight;
				indices[6 * (i * subdivisions + j) + 2] = bottomLeft;
				indices[6 * (i * subdivisions + j) + 3] = topLeft;
				indices[6 * (i * subdivisions + j) + 4] = topRight;
				indices[6 * (i * subdivisions + j) + 5] = bottomRight;
			}
		}
	}
    // Populate vertex buffer with vertices
    glBufferData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    // Generate element array buffer
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // Populate element array buffer with indices
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), indices, GL_STATIC_DRAW);

    // Vertex attribute setup
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    // Delete temporary arrays
    free(vertices);
    free(indices);

    // Bind VAO to zero to unbind
    glBindVertexArray(0);
	Sphere sphere; sphere.vao = VAO; sphere.numIndices = numIndices;sphere.radius = radius;
	return sphere;
}

void drawSphere(Sphere sphere) {
    // Bind VAO
    glBindVertexArray(sphere.vao);
    // Draw sphere
    glDrawElements(GL_TRIANGLES, sphere.numIndices, GL_UNSIGNED_INT, 0);
    // Unbind VAO
    glBindVertexArray(0);
}
