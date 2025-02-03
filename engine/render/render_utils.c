#include "render_utils.h"

#include "logging/log.h"

#include <stdio.h>
#include <stdlib.h>

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

void render_utils_init_glfw(void)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
}

void render_utils_init_glad(void)
{
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        render_utils_crash_game("failed to initialize glad");
    }
    const GLubyte* vendor   = glGetString(GL_VENDOR);      // Returns the vendor
    const GLubyte* renderer = glGetString(GL_RENDERER);    // Returns a hint to the model
    LOG_INFO("Vendor: %s ", vendor);
    LOG_INFO("Renderer: %s ", renderer);
    fflush(stdout);
}

void render_utils_crash_game(char* msg)
{
    LOG_ERROR("Game crashed: %s", msg);
    fflush(stdout);
    exit(1);
}

GLFWwindow* render_utils_create_glfw_window(const char* title, uint32_t width, uint32_t height)
{
    GLFWwindow* window = glfwCreateWindow(width, height, title, glfwGetPrimaryMonitor(), NULL);
    if (window == NULL) {
        render_utils_crash_game("Unable to create glfw window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    // Enable this if you don't want the cursor to be shown
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // Disable V-Sync
    glfwSwapInterval(0);
    return window;
}

// OpengL helper functions
GLuint render_utils_setup_screen_quad(void)
{
    // Vertex data for the rectangle (two triangles forming a quad)
    float vertices[] = {
        // Positions (X, Y)
        -1.0f,
        1.0f,    // Top-left
        -1.0f,
        -1.0f,    // Bottom-left
        1.0f,
        -1.0f,    // Bottom-right

        -1.0f,
        1.0f,    // Top-left
        1.0f,
        -1.0f,    // Bottom-right
        1.0f,
        1.0f    // Top-right
    };
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
    return VAO;
}

GLuint render_utils_setup_debug_cube(void)
{
    GLfloat vertices[] = {
        // Front face
        -0.5f,
        -0.5f,
        0.5f,    // Bottom-left
        0.5f,
        -0.5f,
        0.5f,    // Bottom-right
        0.5f,
        0.5f,
        0.5f,    // Top-right
        -0.5f,
        0.5f,
        0.5f,    // Top-left

        // Back face
        -0.5f,
        -0.5f,
        -0.5f,    // Bottom-left
        0.5f,
        -0.5f,
        -0.5f,    // Bottom-right
        0.5f,
        0.5f,
        -0.5f,    // Top-right
        -0.5f,
        0.5f,
        -0.5f    // Top-left
    };

    // Define the indices for the cube's faces
    GLuint indices[] = {
        // Front face
        0,
        1,
        2,
        2,
        3,
        0,
        // Back face
        4,
        5,
        6,
        6,
        7,
        4,
        // Left face
        0,
        4,
        7,
        7,
        3,
        0,
        // Right face
        1,
        5,
        6,
        6,
        2,
        1,
        // Top face
        3,
        7,
        6,
        6,
        2,
        3,
        // Bottom face
        0,
        4,
        5,
        5,
        1,
        0};

    GLuint VAO, VBO, EBO;
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    // Unbind VAO (NOT EBO) to prevent accidental modification
    glBindVertexArray(0);
    return VAO;
}
