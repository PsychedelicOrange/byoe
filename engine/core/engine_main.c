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
#include "gameobject.h"
#include "logging/log.h"
#include "simd/platform_caps.h"
#include "rng/rng.h"
// Put them at last: causing some weird errors while compiling on MSVC with APIENTRY define
#include <glad/glad.h>
#include <GLFW/glfw3.h>

extern int game_main(void);
// -- -- -- -- -- -- Constants -- -- -- -- -- --- --
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float aspect = (float)SCR_WIDTH/SCR_HEIGHT;
// TODO: remove this later
unsigned int raymarchshader;

// -- -- -- -- -- -- Game state -- -- -- -- -- --- --
// refactor this later into global state/ or expose to script 
vec4 rocks[100];
vec4 rocks_visible[100];
int rocks_visible_count = 0;
// camera settings
float near = 0.01f;
float far = 100.0f;
float fov = GLM_PIf/4;
// debug camera vars (remove when merging pr)
float yaw = -90.f;
float cyaw = -90.f;
float pitch = 0;
vec3s up = {{0,1,0}};
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

void update_first_person_camera(){
	Camera* camera = &gamestate_get_global_instance()->camera;
	vec3s front;
	front.x = cos(glm_rad(yaw)) * cos(glm_rad(pitch));
	front.y = sin(glm_rad(pitch));
	front.z = sin(glm_rad(yaw)) * cos(glm_rad(pitch));
	camera->front = glms_normalize(front);
	camera->right = glms_normalize(glms_cross(camera->front, up));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	camera->up = glms_normalize(glms_cross(camera->right, camera->front));
}

void update_camera_mouse_callback(float xoffset,float yoffset){
	yaw += xoffset;
	pitch += yoffset;
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
}
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	(void)window;
	float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	xoffset *=  0.1;
	yoffset *=  0.1;

	update_camera_mouse_callback(xoffset,yoffset);
}

// -- -- function declare
//
GLFWwindow* create_glfw_window(void);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// -- -- function define
// 
GLFWwindow* create_glfw_window(void) {
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "psychspiration", NULL, NULL);
    if (window == NULL)
    {
        crash_game("Unable to create glfw window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetKeyCallback(window,key_callback);
    // Disable V-Sync
    glfwSwapInterval(0);
    return window;
}

void gl_settings(void) {
    glEnable(GL_DEPTH_TEST);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    (void)window;
	if(glfwGetKey(window,GLFW_KEY_GRAVE_ACCENT)){
		exit(0);
	}
	if(glfwGetKey(window,GLFW_KEY_SPACE)){
		glDeleteProgram(raymarchshader);
		raymarchshader = create_shader("engine/shaders/simple_vert","engine/shaders/raymarch");
		{
			int resolution[2] = {SCR_WIDTH,SCR_HEIGHT};
			setUniformVec2Int(raymarchshader,resolution,"resolution");
			mat4s projection = glms_perspective(fov, (float)SCR_WIDTH / (float)SCR_HEIGHT, near, far);
			setUniformMat4(raymarchshader, projection, "projection");
		}
	}
}

// ig we can just define this function from script and set callback from the script
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)window;
    (void)key;
    (void)scancode;
    (void)action;
    (void)mods;
}
// -- -- -- -- -- -- -- -- --


void print_vec3(float* vec){
	printf("(%f,%f,%f)",vec[0],vec[1],vec[2]);
}
// debug randomize rocks 
void debug_randomize_rocks(){
	rng_generate();
	for(int i = 0; i < 100; i++){
		rocks[i][0] = (float)rng_range(1,10);
		rocks[i][1] = (float)rng_range(1,10);
		rocks[i][2] = (float)rng_range(1,10);
		rocks[i][0] -= 5;
		rocks[i][1] -= 5;
		rocks[i][2] -= 5;
		rocks[i][3] = 0.5;
	}
	rocks[1][0] = 1;
	rocks[1][1] = 1;
	rocks[1][2] = -2;
	rocks[0][0] = 1;
	rocks[0][1] = 1;
	rocks[0][2] = -6;
}

int canBeVisible(vec4 p,vec3 point_on_plane,vec3 normal){
	float distance = glm_vec3_dot(normal,point_on_plane);
	return (glm_vec3_dot(normal, p)) - distance > -p[3] - 1.25f;
}

// taken from https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
typedef struct Matrix4x4
{
	// The elements of the 4x4 matrix are stored in
	// column-major order (see "OpenGL Programming Guide",
	// 3rd edition, pp 106, glLoadMatrix).
	float _11, _21, _31, _41;
	float _12, _22, _32, _42;
	float _13, _23, _33, _43;
	float _14, _24, _34, _44;
}Matrix4x4;

typedef struct Plane
{
	float a, b, c, d;
}Plane;

typedef struct Frustum{
	Plane planes[6];
}Frustum;

void NormalizePlane(Plane plane)
{
	float mag;
	mag = sqrt(plane.a * plane.a + plane.b * plane.b + plane.c * plane.c);
	plane.a = plane.a / mag;
	plane.b = plane.b / mag;
	plane.c = plane.c / mag;
	plane.d = plane.d / mag;
}

void ExtractPlanesGL(
		Plane * p_planes,
		const Matrix4x4 comboMatrix,
		bool normalize)
{
	// Left clipping plane
	p_planes[0].a = comboMatrix._41 + comboMatrix._11;
	p_planes[0].b = comboMatrix._42 + comboMatrix._12;
	p_planes[0].c = comboMatrix._43 + comboMatrix._13;
	p_planes[0].d = comboMatrix._44 + comboMatrix._14;
	// Right clipping plane
	p_planes[1].a = comboMatrix._41 - comboMatrix._11;
	p_planes[1].b = comboMatrix._42 - comboMatrix._12;
	p_planes[1].c = comboMatrix._43 - comboMatrix._13;
	p_planes[1].d = comboMatrix._44 - comboMatrix._14;
	// Top clipping plane
	p_planes[2].a = comboMatrix._41 - comboMatrix._21;
	p_planes[2].b = comboMatrix._42 - comboMatrix._22;
	p_planes[2].c = comboMatrix._43 - comboMatrix._23;
	p_planes[2].d = comboMatrix._44 - comboMatrix._24;
	// Bottom clipping plane
	p_planes[3].a = comboMatrix._41 + comboMatrix._21;
	p_planes[3].b = comboMatrix._42 + comboMatrix._22;
	p_planes[3].c = comboMatrix._43 + comboMatrix._23;
	p_planes[3].d = comboMatrix._44 + comboMatrix._24;
	// Near clipping plane
	p_planes[4].a = comboMatrix._41 + comboMatrix._31;
	p_planes[4].b = comboMatrix._42 + comboMatrix._32;
	p_planes[4].c = comboMatrix._43 + comboMatrix._33;
	p_planes[4].d = comboMatrix._44 + comboMatrix._34;
	// Far clipping plane
	p_planes[5].a = comboMatrix._41 - comboMatrix._31;
	p_planes[5].b = comboMatrix._42 - comboMatrix._32;
	p_planes[5].c = comboMatrix._43 - comboMatrix._33;
	p_planes[5].d = comboMatrix._44 - comboMatrix._34;
	// Normalize the plane equations, if requested
	if (normalize == true)
	{
		NormalizePlane(p_planes[0]);
		NormalizePlane(p_planes[1]);
		NormalizePlane(p_planes[2]);
		NormalizePlane(p_planes[3]);
		NormalizePlane(p_planes[4]);
		NormalizePlane(p_planes[5]);
	}

}

int ClassifySphere(const Plane plane, vec4 pt)
{
	float d;
	d = plane.a*pt[0] + plane.b*pt[1] + plane.c*pt[2] + plane.d;
	return d > -pt[3] - 1.25f;
}

void cull_rocks(){
	rocks_visible_count = 0;
	// construct frustum
	Frustum f;
	mat4s projection = glms_perspective(fov, (float)SCR_WIDTH / (float)SCR_HEIGHT, near, far);
	mat4s viewproj = glms_mul(projection,gamestate_get_global_instance()->camera.lookAt);
	Matrix4x4 mat; 
	memcpy(&mat,viewproj.raw,sizeof(float)*16);
	ExtractPlanesGL( f.planes, mat, true);
	
	for(int i = 0; i < 100; i++){
		int visible = 
			ClassifySphere(f.planes[0],rocks[i]) &&
			ClassifySphere(f.planes[1],rocks[i]) && 
			ClassifySphere(f.planes[2],rocks[i]) && 
			ClassifySphere(f.planes[3],rocks[i]) && 
			ClassifySphere(f.planes[4],rocks[i]) && 
			ClassifySphere(f.planes[5],rocks[i]);
		if(visible){
			memcpy(rocks_visible[rocks_visible_count++],rocks[i],sizeof(float)*4);
		}
	}
	printf("\rRocksVisible: %i",rocks_visible_count);
	fflush(stdout);
}

void frustumCull(){
	rocks_visible_count = 0;
	Camera camera = gamestate_get_global_instance()->camera;
	float halfVSide = far * tanf(fov/2);
	float halfHSide = halfVSide * aspect;	
	vec3s frontMultFar = glms_vec3_scale(camera.front,far);
	for(int i = 0; i < 100; i++){
		int visible = 
			canBeVisible(rocks[i], glms_vec3_add(camera.position,glms_vec3_scale(camera.front,near)).raw, camera.front.raw) && //near
			canBeVisible(rocks[i], glms_vec3_add(camera.position,frontMultFar).raw, glms_normalize(glms_vec3_scale(camera.front,-1)).raw) &&
			canBeVisible(rocks[i], camera.position.raw, glms_vec3_cross(glms_vec3_sub(frontMultFar,glms_vec3_scale(camera.right,halfHSide)),camera.up).raw )&& //right
			canBeVisible(rocks[i], camera.position.raw, glms_vec3_cross(camera.up,glms_vec3_add(frontMultFar,glms_vec3_scale(camera.right,halfHSide))).raw )&& //left
			canBeVisible(rocks[i], camera.position.raw, glms_vec3_cross(camera.right,glms_vec3_sub(frontMultFar,glms_vec3_scale(camera.up,halfHSide))).raw )&&//top
			canBeVisible(rocks[i], camera.position.raw, glms_vec3_cross(glms_vec3_add(frontMultFar,glms_vec3_scale(camera.up,halfHSide)),camera.right).raw ); //bottom
		if(visible){
			memcpy(rocks_visible[rocks_visible_count++],rocks[i],sizeof(float)*4);
		}
	}
	printf("\rRocksVisible: %i",rocks_visible_count);
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    srand((unsigned int)time(NULL));  // Seed for random number generator

    cpu_caps_print_info();
    os_caps_print_info();

    init_glfw();
    GLFWwindow* window = create_glfw_window();
    init_glad();
    gl_settings();
    unsigned int shaderProgram = create_shader("engine/shaders/vertex","engine/shaders/frag");
    raymarchshader = create_shader("engine/shaders/simple_vert","engine/shaders/raymarch");
	// set constant shader variables
	{
		int resolution[2] = {SCR_WIDTH,SCR_HEIGHT};
		setUniformVec2Int(raymarchshader,resolution,"resolution");
	}

	debug_randomize_rocks();

	GLuint screen_quad_vao = setup_screen_quad();
	GLuint vao = setup_debug_cube();

    // set uniforms
    {
        mat4s projection = glms_perspective(fov, (float)SCR_WIDTH / (float)SCR_HEIGHT, near, far);
        setUniformMat4(shaderProgram, projection, "projection");
        setUniformMat4(raymarchshader, projection, "projection");
        mat4s model = GLMS_MAT4_IDENTITY_INIT;
        setUniformMat4(shaderProgram, model, "model");
    }

    //////////////////////////////////////////////////////// 
    // START GAME RUNTIME
    init_game_registry();

    // register game objects on run-time side
    game_main();

    // game scripting start function
    gameobjects_start();
    ////////////////////////////////////////////////////////

    // render loop
    // -----------
    float deltaTime; // Time between current frame and last frame
    float lastFrame = 0.0f; // Time of the last frame
    float elapsedTime = 0;

    int FPS = 0;

    while (!glfwWindowShouldClose(window))
    {
        // Get current time
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame; // Calculate delta time
        // calculate FPS
        FPS = (int)(1.0f / deltaTime);
		
		update_first_person_camera();

        elapsedTime += deltaTime;
        if (elapsedTime > 1.0f) {
            char windowTitle[250];
            sprintf(windowTitle, "BYOE Game: byoe_ghost_asteroids | FPS: %d | rendertime: %2.2fms", FPS, deltaTime * 1000.0f);
            glfwSetWindowTitle(window, windowTitle);
            elapsedTime = 0.0f;
        }

        processInput(window);

        // Update the global game state
        gamestate_update(window);

        // Game scripts update loop
        gameobjects_update(deltaTime);


        // Render
        // ------
		// frustum cull
		frustumCull();
		//cull_rocks();
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Draw cubes
        {
            for (size_t i = 0; i < game_registry_get_instance()->capacity; i++)
            {
                hash_map_pair_t pair = game_registry_get_instance()->entries[i];
                if (!uuid_is_null(&pair.key) && pair.value) {
                    GameObject* go = (GameObject*)pair.value;
                    if (strcmp(go->typeName, "Camera") == 0)
                        continue;

                    // LOG_INFO("rendering gameobject: %s position: (%f, %f, %f)\n", go->typeName, go->transform.position[0], go->transform.position[1], go->transform.position[2]);

                    // Render
                    glUseProgram(shaderProgram);
                    setUniformMat4(shaderProgram, gamestate_get_global_instance()->camera.lookAt, "view");
                    mat4s model;
                    model = gameobject_ptr_get_transform(go);

                    setUniformMat4(shaderProgram, model, "model");

                    glBindVertexArray(vao);
                    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                    glBindVertexArray(0);
                }
            }
        }
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		{
			glUseProgram(raymarchshader);
			int loc = glGetUniformLocation(raymarchshader,"rocks");
			glUniform4fv(loc,rocks_visible_count,&rocks_visible[0][0]);	
			loc = glGetUniformLocation(raymarchshader,"campos");
			glUniform3fv(loc,1,&(gamestate_get_global_instance()->camera.position.raw[0]));	
			loc = glGetUniformLocation(raymarchshader,"rocks_count");
			glUniform1i(loc,rocks_visible_count);
			setUniformMat4(raymarchshader, gamestate_get_global_instance()->camera.lookAt, "view");

			glBindVertexArray(screen_quad_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);  // Drawing 6 vertices to form the quad
		}
		//(void)screen_quad_vao;
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        lastFrame = currentFrame; // Update last frame time
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


