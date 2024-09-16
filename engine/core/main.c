#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <cglm/cglm.h>   /* for inline */

char* readFileToString(const char* filename) {
    // Open the file in read mode ("r")
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Could not open file %s\n", filename);
        return NULL;
    }

    // Move the file pointer to the end of the file to determine file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);  // Move file pointer back to the beginning

    // Allocate memory for the file content (+1 for the null terminator)
    char *content = (char*)malloc((fileSize + 1) * sizeof(char));
    if (!content) {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read file contents into the string
    fread(content, sizeof(char), fileSize, file);
    content[fileSize] = '\0';  // Null-terminate the string

    // Close the file and return the content
    fclose(file);
    return content;
}

vec3 up = {0,1,0};
typedef struct Camera{
	mat4 lookAt;
	vec3 position;
	vec3 right;
	vec3 front;
}Camera;

// -- -- -- -- -- -- Function declare -- -- -- -- -- --- --



void crash_game(char* msg);

void init_glfw();
GLFWwindow* create_glfw_window();
void init_glad();

unsigned int compile_shader(const char* source, int shaderType);
unsigned int create_program(unsigned int vertexShader,unsigned int fragmentShader);

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
// -- -- -- -- -- -- Contants -- -- -- -- -- --- --
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// -- -- -- -- -- -- GLFW Functions -- -- -- -- -- --- --
void init_glfw(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

}

GLFWwindow* create_glfw_window(){
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "psychspiration", NULL, NULL);
    if (window == NULL)
    {
		crash_game("Unable to create glfw window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window,key_callback);
	return window;
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
// -- -- -- -- -- -- Shader functions -- -- -- -- -- --- --
//
unsigned int compile_shader(const char * shaderCode, int shaderType){
	unsigned int shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderCode, NULL);
	glCompileShader(shader);
	// check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		printf("failed to compile shader: %s", infoLog);
		return -1;
	}
	return shader;
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
		printf("failed to link shaders: %s",infoLog);
		return -1;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
	return shaderProgram;
}

// -- -- -- -- -- -- Engine Helpers Functions -- -- -- -- -- --- --
void crash_game(char* msg){
	printf("\nGame crashed: %s\n",msg);
	fflush(stdout);
	exit(1);
}
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

	GLuint *VAO,*VBO,*EBO;
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glGenBuffers(1, EBO);

    // Bind VAO
    glBindVertexArray(*VAO);

    // Bind and set VBO
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Bind and set EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind VAO (NOT EBO) to prevent accidental modification
    glBindVertexArray(0);
return *VAO;
}
// -- -- -- -- -- -- Math functions -- -- -- -- - -
//
int main()
{
	init_glfw();
	GLFWwindow* window = create_glfw_window();
	init_glad();
	
	unsigned int shaderProgram;
	{
		unsigned int vertexShader  = compile_shader(readFileToString("engine/shaders/vertex"),GL_VERTEX_SHADER);
		unsigned int fragShader  = compile_shader(readFileToString("engine/shaders/frag"),GL_FRAGMENT_SHADER);
		shaderProgram = create_program(vertexShader,fragShader);
	}

	Camera camera;

	GLuint vao = setup_debug_cube();

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
		glm_lookat(camera.position,camera.front,up,camera.lookAt);
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

		// draw cube
		glUseProgram(shaderProgram);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    //glDeleteVertexArrays(1, &vao[0]);
    //glDeleteBuffers(1, &vbo[0]);
    //glDeleteBuffers(1, &ebo[0]);
    //glDeleteProgram(environmentShader);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
	printf("\nbye!\n");
    return 0;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// ig we can just set this from the script
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		
	}
}
