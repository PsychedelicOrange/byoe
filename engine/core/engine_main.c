#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <cglm/cglm.h>   /* for inline */
#include <cglm/struct.h> /* struct api */

#include "shader.h"
#include "utils.h"
#include "game_state.h"
#include "scripting.h"
#include "game_registry.h"

extern int game_main(void);

// -- -- -- -- -- -- Contants -- -- -- -- -- --- --
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
vec3s up = {{0,1,0}};
float speed = 0.1;


typedef struct Camera{
	mat4s lookAt;
	vec3s position;
	vec3s right;
	vec3s front;
}Camera;
Camera camera;


// -- -- function declare
//
GLFWwindow* create_glfw_window(void);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);


// -- -- function define
// 
GLFWwindow* create_glfw_window(void){
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "psychspiration", NULL, NULL);
    if (window == NULL)
    {
		crash_game("Unable to create glfw window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//glfwSetKeyCallback(window,key_callback);
	return window;
}

void gl_settings(void){
	glEnable(GL_DEPTH_TEST);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    (void)window;
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.position = glms_vec3_add(camera.position,glms_vec3_scale_as(camera.front,speed));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.position = glms_vec3_add(camera.position,glms_vec3_scale_as(camera.right,-speed));
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.position = glms_vec3_add(camera.position,glms_vec3_scale_as(camera.front,-speed));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.position = glms_vec3_add(camera.position,glms_vec3_scale_as(camera.right,speed));
}

// ig we can just define this function from script and set callback from the script
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)window;
    (void) scancode;
    (void) mods;

    if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		camera.position.x -= speed;
	}
}
// -- -- -- -- -- -- -- -- --

GLuint setup_debug_cube(void){
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

void init_camera(void){
	camera.position.x = 0;
	camera.position.y = 0;
	camera.position.z = 3;

	camera.front.x = 0;
	camera.front.y = 0;
	camera.front.z = -1;

	camera.right.x = 1;
	camera.right.y = 0;
	camera.right.z = 0;
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

	init_glfw();
	GLFWwindow* window = create_glfw_window();
	init_glad();
	gl_settings();
	unsigned int shaderProgram;
	{
		unsigned int vertexShader  = compile_shader("engine/shaders/vertex",GL_VERTEX_SHADER);
		unsigned int fragShader  = compile_shader("engine/shaders/frag",GL_FRAGMENT_SHADER);
		shaderProgram = create_program(vertexShader,fragShader);
	}

	GLuint vao = setup_debug_cube();

	// set uniforms
	{
		mat4s projection = glms_perspective(glm_rad(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		setUniformMat4(shaderProgram,projection,"projection");
		mat4s model = GLMS_MAT4_IDENTITY_INIT;
		setUniformMat4(shaderProgram,model,"model");
	}

	init_camera();

    //////////////////////////////////////////////////////// 
    // START GAME RUNTIME
    init_game_registry();

    game_main();

    gameobjects_start();
    ////////////////////////////////////////////////////////

    // render loop
    // -----------
    float deltaTime; // Time between current frame and last frame
    float lastFrame = 0.0f; // Time of the last frame

    while (!glfwWindowShouldClose(window))
    {
        // Get current time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame; // Calculate delta time
        lastFrame = currentFrame; // Update last frame time

        processInput(window);

        // TODO: Update Game State here with input polling
        // TESTING UPDATE LOOP
        gameobjects_update(deltaTime);

        glm_look(camera.position.raw, camera.front.raw, up.raw, camera.lookAt.raw);

        // Render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw cube
        glUseProgram(shaderProgram);
        setUniformMat4(shaderProgram, camera.lookAt, "view");

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanup_game_registry();
    

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


