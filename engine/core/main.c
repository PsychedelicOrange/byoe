#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <cglm/cglm.h>   /* for inline */
#include <cglm/struct.h> /* struct api */
#include <string.h>
#include "shader.h"
#include "utils.h"
#include "shapes.h"
// -- -- -- -- -- -- Contants -- -- -- -- -- --- --
// Origin cube transform
mat4s og = GLMS_MAT4_IDENTITY_INIT;
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
vec3s up = {{0,1,0}};
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// -- -- -- -- -- -- Globals Variables -- -- -- -- -- --- --
// input keys
float last_pressed[GLFW_KEY_LAST] = {0};
int keys[GLFW_KEY_LAST] = {0};
float direction_thrust = 0;
float forward_thrust = 0;
// -- -- -- -- -- -- Objects -- -- -- -- -- --- --
// Hero
vec3s position = GLMS_VEC3_ZERO;
vec3s speed = GLMS_VEC3_ZERO;
float pRadius = 1;


// delta time
float last_frame = 0, delta_time = 0;
void update_delta_time(){
	float current_frame = glfwGetTime();
	delta_time = current_frame - last_frame;
	last_frame = current_frame;
}

// Camera
// -- -- -- -- -- -- -- -- --
typedef struct Camera{
	mat4s lookAt;
	vec3s position;
	vec3s right;
	vec3s front;
	vec3s up;
}Camera;
Camera camera;
float yaw = -90.f;
float pitch = 0;

// cobjects
typedef struct cObject{
	vec3s position;
	float radius;
}cObject;

void update_third_person_camera(){
	yaw += direction_thrust;
	camera.position.x = cos(glm_rad(pitch))*(sqrt(75))*cos(glm_rad(yaw)) + position.x;
	camera.position.y = sin(glm_rad(pitch))*(sqrt(75)) + position.y;
	camera.position.z = cos(glm_rad(pitch))*(sqrt(75))*sin(glm_rad(yaw)) + position.z;

	camera.front = glms_normalize(glms_vec3_sub(camera.position,position));
	camera.right = glms_normalize(glms_cross(camera.front, up));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	camera.up = glms_normalize(glms_cross(camera.right, camera.front));

	printf("\r Camera.position: %f, %f, %f",camera.position.x,camera.position.y, camera.position.z);
}

void update_first_person_camera(float xoffset,float yoffset){
	vec3s front;
	front.x = cos(glm_rad(yaw)) * cos(glm_rad(pitch));
	front.y = sin(glm_rad(pitch));
	front.z = sin(glm_rad(yaw)) * cos(glm_rad(pitch));
	camera.front = glms_normalize(front);
	camera.right = glms_normalize(glms_cross(camera.front, up));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	camera.up = glms_normalize(glms_cross(camera.right, camera.front));
}

void update_camera_mouse_callback(float xoffset,float yoffset){
	//yaw += xoffset;
	pitch -= yoffset;
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < 0)
		pitch = 0;
}


// -- callbacks
void framebuffer_size_callback(GLFWwindow *window, int width, int height){glViewport(0, 0, width, height);}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	switch(action){
		case GLFW_PRESS:
			last_pressed[key] = glfwGetTime();
			keys[key] = action;
			break;
		case GLFW_RELEASE:
			keys[key] = action;
			break;
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	xoffset *=  0.1;
	yoffset *=  0.1;

	update_camera_mouse_callback(xoffset,yoffset);
}

// set callbacks
GLFWwindow* create_glfw_window(){
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "psychspiration", NULL, NULL);
    if (window == NULL) {
		crash_game("Unable to create glfw window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window,key_callback);
	return window;
}

void gl_settings(){
	glEnable(GL_DEPTH_TEST);
}

float lerp(float a, float b, float t){return a + ((b-a) * t);}
float damper_exact(float x, float g, float halflife, float dt){
	const float eps=1e-5f;
    return lerp(x, g, 1.0f - expf(-(0.69314718056f * dt) / (halflife + eps)));
}

void calc_translation(){
	vec3s translate = GLMS_VEC3_ZERO;
	translate = glms_vec3_scale(speed,delta_time);
	position = glms_vec3_add(position,translate);
}

void processInput() {
	//printf("\rSpeed: %f, Accel : %f",speed.z,acceleration.z);
	fflush(stdout);
	forward_thrust = 0;
	int thrust_key_pressed = 0;
	direction_thrust=0;
	int direction_key_pressed =0;
	float start_speed = 5;
	float max_speed = 30;

	if(keys[GLFW_KEY_W] == GLFW_PRESS){// if key is being pressed (last captured event was being pressed)
		float time_held = glfwGetTime() - last_pressed[GLFW_KEY_W];
		thrust_key_pressed = 1;
		forward_thrust = -fmin( start_speed + delta_time*((time_held*time_held*time_held) * 100), max_speed);//-damper_exact(forward_thrust,max_speed,1,delta_time);
		//forward_thrust = const_speed * delta_time;
	}
	if(keys[GLFW_KEY_S] == GLFW_PRESS){
		thrust_key_pressed = 1;
		forward_thrust = damper_exact(start_speed + forward_thrust,max_speed,1,delta_time);
	}
	if(keys[GLFW_KEY_A] == GLFW_PRESS){
		direction_key_pressed =1;
		direction_thrust = -100 * delta_time ;
	}
	if(keys[GLFW_KEY_D] == GLFW_PRESS){
		direction_key_pressed =1;
		direction_thrust= 100* delta_time ;
	}

	vec3s front;
	front.x = camera.front.x;
	front.y = 0;
	front.z = camera.front.z;
	front = glms_vec3_normalize(front);

	if(!thrust_key_pressed){
		forward_thrust = damper_exact(forward_thrust,0,0.5,delta_time);
		direction_thrust = 0;
	}else{
		yaw += direction_thrust;
	}

	speed = glms_vec3_scale(front,forward_thrust);

	if(keys[GLFW_KEY_ESCAPE] == GLFW_PRESS){
		exit(1);
	}

}

// -- collision

vec3s collide(vec3s hpos,float hradius,cObject object){
	if(glms_vec3_distance(object.position,hpos) < object.radius+hradius){
		hpos = glms_vec3_add(object.position, glms_vec3_scale(glms_vec3_normalize(glms_vec3_sub(hpos,object.position)),object.radius+hradius));
	}
	return hpos;
}

cObject setupObstacle (float position[3], float radius){
	cObject obs;
	memcpy(obs.position.raw,position,sizeof(float)*3);
	obs.radius = radius;
	return obs;
}

void init_camera(){
	camera.position.x = 0;
	camera.position.y = 1;
	camera.position.z = 5;

	camera.front.x = 0;
	camera.front.y = 0;
	camera.front.z = -1;

	camera.right.x = 1;
	camera.right.y = 0;
	camera.right.z = 0;
}

int main()
{
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
	Sphere sphere = generateSphere(pRadius,50);

	// set uniforms
	{
		mat4s projection = glms_perspective(glm_rad(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		mat4s cube_transform = GLMS_MAT4_IDENTITY_INIT;
		setUniformMat4(shaderProgram,projection,"projection");
		setUniformMat4(shaderProgram,cube_transform,"model");
	}

	init_camera();
	og = glms_scale_uni(og,0.1);
	float obspos[3] = {8,0,8};
	cObject obs = setupObstacle(obspos,5);

    while (!glfwWindowShouldClose(window))
    {
		// game stuff
		update_delta_time();
		processInput();
		position = collide(position,pRadius,obs);
		calc_translation();
		
		update_third_person_camera();

		glm_lookat(camera.position.raw,position.raw,camera.up.raw,camera.lookAt.raw);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw pango
		{
			mat4s cube_transform = GLMS_MAT4_IDENTITY_INIT;
			cube_transform = glms_translate(cube_transform,position);
			setUniformMat4(shaderProgram,cube_transform,"model");
			setUniformMat4(shaderProgram,camera.lookAt,"view");
			glUseProgram(shaderProgram);
			drawSphere(sphere);
		}

		// lets add an obstacle
		//
		{
			mat4s obstacleT = GLMS_MAT4_IDENTITY_INIT;
			obstacleT = glms_translate(obstacleT,obs.position);
			obstacleT = glms_scale_uni(obstacleT,obs.radius);
			setUniformMat4(shaderProgram,obstacleT,"model");
			glUseProgram(shaderProgram);
			drawSphere(sphere);
		}


		glBindVertexArray(vao);
		setUniformMat4(shaderProgram,og,"model");
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
	printf("\nbye!\n");
    return 0;
}
