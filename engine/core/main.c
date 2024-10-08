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
#include "model_loading.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

int rand_uni(int min,int max){return ((double) rand() / (RAND_MAX+1)) * (max-min+1) + min;}
float lerp(float a, float b, float t){return a + ((b-a) * t);}
float damper_exact(float x, float g, float halflife, float dt){
	const float eps=1e-5f;
    return lerp(x, g, 1.0f - expf(-(0.69314718056f * dt) / (halflife + eps)));
}
	// Function to draw the health bar with color interpolation
	void drawHealthBar(float healthPercent, float x, float y, float width, float height, GLuint shaderProgram) {
		// Ensure healthPercent is between 0 and 1
		if (healthPercent > 1.0f) healthPercent = 1.0f;
		if (healthPercent < 0.0f) healthPercent = 0.0f;

		// Vertices for a rectangle (normalized device coordinates), including colors
		float vertices[] = {
			// Position          // Color (r, g, b)
			x, y,                1.0f - healthPercent, healthPercent, 0.0f,  // Bottom-left corner (Color interpolates based on health)
			x + width * healthPercent, y,    1.0f - healthPercent, healthPercent, 0.0f,  // Bottom-right corner
			x + width * healthPercent, y + height, 1.0f - healthPercent, healthPercent, 0.0f,  // Top-right corner
			x, y + height,       1.0f - healthPercent, healthPercent, 0.0f   // Top-left corner
		};

		// Indices for drawing the rectangle using two triangles
		unsigned int indices[] = {
			0, 1, 2,  // First triangle
			2, 3, 0   // Second triangle
		};

		GLuint VBO, VAO, EBO;
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

		// Define position attribute (2 floats for x and y)
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// Define color attribute (3 floats for r, g, b)
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// Use the shader program
		glUseProgram(shaderProgram);
		float green = healthPercent;
		float red = 1.0f - healthPercent;
		glUniform3f(glGetUniformLocation(shaderProgram, "color"), red, green, 0.0f);
		// Draw the health bar
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Unbind everything
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		// Delete the buffers
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
	}

void actuallydrawHealthBar(unsigned int shaderProgram,unsigned int VAO, float healthPercent){
glUseProgram(shaderProgram);

// Draw the health bar
glBindVertexArray(VAO);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
// -- -- -- -- -- -- Contants -- -- -- -- -- --- --
// Origin cube transform
mat4s og = GLMS_MAT4_IDENTITY_INIT;
// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
vec3s up = {{0,1,0}};
float lastX = 1920 / 2.0f;
float lastY = 1080 / 2.0f;

// -- -- -- -- -- -- Globals Variables -- -- -- -- -- --- --
// input keys
int already_pressed = 0;
float last_pressed[GLFW_KEY_LAST] = {0};
int keys[GLFW_KEY_LAST] = {0};
float direction_thrust = 0;
float forward_thrust = 0;
// Textures
void set_shader_texture_units(GLuint shaderProgram){
	setUniformInt(shaderProgram,GL_TEXTURE0,"base_color");
	setUniformInt(shaderProgram,GL_TEXTURE1,"normal");
	setUniformInt(shaderProgram,GL_TEXTURE2,"rough_metal");
}
typedef struct cObject{
	vec3s position;
	float radius;
}cObject;
typedef struct bullet{
	vec3s direction;
	vec3s position;
	int active;
}bullet;
// game varialbes
bullet bullets[10];
int health = 100;
int level = 0;
int mode = 0;
vec3 anthill_location[3] = {{2,0,4},
{26.27943992614746,2.9802322387695312e-08, -1.945894479751587},
{-23.67688751220703,2.9802322387695312e-08, 23.10959243774414}};
float anthill_time[3] = {0};
int anthill_bested[3] = {0};

vec3 guns[10] = {
	{1.2031736373901367, 0, 52.12920379638672},
	{51.312007904052734, 0, -9.272510528564453},
	{-26.360157012939453,0, 44.98937225341797},
	{-37.82976531982422, 0, 35.886077880859375},
	{50.43590545654297, 0, 13.233322143554688},
	{52.12920379638672, 0, 1.2031736373901367},
	{-9.272510528564453, 0, 51.312007904052734},
	{44.98937225341797,0, - 26.360157012939453},
	{35.886077880859375,0,-37.82976531982422},
	{13.233322143554688, 0, 50.43590545654297}

};
int guns_bested[10] = {100,100,100,100,100,100,100,100,100,100};

//-- -- -- -- -- -- Objects -- -- -- -- -- --- --
// Hero
vec3s position;
vec3s speed;

float pRadius = 0.7;
float angle = 0;
primitive_env pango_ball_model;
primitive_env pango_model;
bullet myBullets[5];


// Environments
primitive_env grass;
primitive_env leaf;
primitive_env bark;
primitive_env tree[2];
drawable_mesh tree_m[2];
GLuint tree_ssbo;
primitive_env anthill;
cObject tree_collider[11] ; 

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
float cyaw = -90.f;
float pitch = 0;


void update_third_person_camera(){
	cyaw = damper_exact(cyaw,yaw,1,delta_time);
	camera.position.x = cos(glm_rad(pitch))*(sqrt(75))*cos(glm_rad(yaw)) + position.x;
	camera.position.y = sin(glm_rad(pitch))*(sqrt(75)) + position.y;
	camera.position.z = cos(glm_rad(pitch))*(sqrt(75))*sin(glm_rad(yaw)) + position.z;

	camera.front = glms_normalize(glms_vec3_sub(camera.position,position));
	camera.right = glms_normalize(glms_cross(camera.front, up));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	camera.up = glms_normalize(glms_cross(camera.right, camera.front));

	//printf("\r Camera.right: %f, %f, %f",camera.right.x,camera.right.y, camera.right.z);
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
	yaw += xoffset;
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
	if(key == GLFW_KEY_SPACE && action == GLFW_PRESS){
		mode = !mode;
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
// set callbacks
GLFWwindow* create_glfw_window(){
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "psychspiration", glfwGetPrimaryMonitor(), NULL);
    if (window == NULL) {
		crash_game("Unable to create glfw window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window,key_callback);
	srand(glfwGetTime());
	return window;
}

void gl_settings(){
	glEnable(GL_DEPTH_TEST);
}


void calc_translation(){
	vec3s translate = GLMS_VEC3_ZERO;
	translate = glms_vec3_scale(speed,delta_time);
	position = glms_vec3_add(position,translate);
}

void processInput(GLFWwindow* window) {
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if (state == GLFW_PRESS){
		
	}
	//printf("\rSpeed: %f, Accel : %f",speed.z,acceleration.z);
	fflush(stdout);
	//forward_thrust = 0;
	int thrust_key_pressed = 0;
	direction_thrust=0;
	int direction_key_pressed =0;
	float start_speed = 5;
	float max_speed = mode ? 10 : 30;

	if(keys[GLFW_KEY_W] == GLFW_PRESS){// if key is being pressed (last captured event was being pressed)
		float time_held = glfwGetTime() - last_pressed[GLFW_KEY_W];
		thrust_key_pressed = 1;
		//forward_thrust = -fmin( start_speed + delta_time*((time_held*time_held*time_held) * 100), max_speed);//-damper_exact(forward_thrust,max_speed,1,delta_time);
		forward_thrust = damper_exact(forward_thrust,-max_speed,1,delta_time);
		//forward_thrust = const_speed * delta_time;
	}
	if(keys[GLFW_KEY_S] == GLFW_PRESS){
		thrust_key_pressed = 1;
		forward_thrust = damper_exact(forward_thrust,max_speed,1,delta_time);
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
		forward_thrust = damper_exact(forward_thrust,0,0.1,delta_time);
		direction_thrust = 0;
	}else{
		yaw += direction_thrust;
	}

	angle += (forward_thrust) / pRadius;
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
	if(glms_vec3_norm(hpos) > 10){
		position.x = 0; position.y =0; position.z = 0;
	}
	hpos.y = hradius;
	return hpos;
}
void check_ant_hill_collision(vec3s hpos, float hradius){
	if (mode == 0)return;
	vec3s front;
	front.x = camera.front.x;
	front.y = 0;
	front.z = camera.front.z;
	front = glms_vec3_normalize(front);
	hpos = glms_vec3_add(hpos,glms_vec3_scale(front,-1.5));
	hpos.y =0;
	hradius = 0.1;
	for(int i = 0; i < 3; i++){
		if(glm_vec3_distance(anthill_location[i],hpos.raw) < 0.1+hradius){
			
			health += delta_time*100;
			if (health > 100) health = 100;
		}	
	}
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

	position.x = 0;
	position.y = 0;
	position.z = 0;
	speed.x = 0;
	speed.y = 0;
	speed.z = 0;

	camera.front.x = 0;
	camera.front.y = 0;
	camera.front.z = -1;

	camera.right.x = 1;
	camera.right.y = 0;
	camera.right.z = 0;
}

void generate_trees(int n){

	vec3 tpos[11] = {
		{-22.425683975219727, 23.070524215698242, 0.0},
		{-38.162391662597656, -23.888139724731445, 0.0},
		{-39.91145324707031, 6.732744216918945, 0.0},
		{19.23278045654297, -18.151504516601562, 0.0},
		{-9.21585464477539, -38.73683166503906, 0.0},
		{-36.47857666015625, 36.37974548339844, 0.0},
		{8.952789306640625, 42.019500732421875, 0.0},
		{42.791324615478516, 27.92011260986328, 0.0},
		{42.321346282958984, -15.161354064941406, 0.0},
		{24.6187801361084, -1.8452653884887695, 0.0},
		{40.284767150878906, 51.575748443603516, 0.0}
	};
	// generate positions and add collision objects
	mat4* transforms = malloc(sizeof(mat4)*n);
	for(int i =0 ; i < n; i++){
		mat4 transform = GLM_MAT4_IDENTITY_INIT;
		vec3 t = {tpos[i][0],tpos[i][2],tpos[i][1]};
		glm_translate(transform,t);
		glm_rotate(transform,glm_rad(rand()%100),up.raw);
		//glm_scale_uni(transform,10);
		glm_mat4_copy(transform,transforms[i]);
		tree_collider[i] = setupObstacle(t,1);
	}

	// upload buffer
	glGenBuffers(1, &tree_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tree_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(mat4) * n, transforms, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, tree_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	free(transforms);

}

void draw_trees(unsigned int shaderProgram, int n){
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, tree_ssbo);
	glUseProgram(shaderProgram);
	bind_textures(tree_m[0].mat);
	glBindVertexArray(tree_m[0].vao);
	for(int i =0 ; i < n; i++){
		glDrawElementsInstanced(GL_TRIANGLES, tree_m[0].indices_count,GL_UNSIGNED_SHORT, 0,n);
	}
	bind_textures(tree_m[1].mat);
	glBindVertexArray(tree_m[1].vao);
	for(int i =0 ; i < n; i++){
		glDrawElementsInstanced(GL_TRIANGLES, tree_m[1].indices_count,GL_UNSIGNED_SHORT, 0,n);
	}
}
void shoot_bullets(){
	for(int i = 0 ; i < 10; i++){
		bullets[i].position.x = guns[i][0];
		bullets[i].position.y = guns[i][1];
		bullets[i].position.z = guns[i][2];
		bullets[i].direction = glms_vec3_normalize(glms_vec3_sub(bullets[i].position,position));
		bullets[i].active = 1;
	}
}

void collide_bullets(vec3s hpos, float hradius){
	for(int i =0 ; i < 10; i ++){
		if(bullets[i].active == 0)continue;
		// collided with pango
		if(glms_vec3_distance(bullets[i].position,hpos) < 0.2+hradius){
			health -= 35;
			if (health < 0) health = 0;
			//printf("\t \t Damage taken: ch ; %i",health);
			bullets[i].active = 0;
			continue;
		}
		// collided with tree
		for(int i =0 ; i < 10; i ++){
			if(myBullets[i].active == 0)continue;
			for(int t = 0; t < 11; t++){
				if(glms_vec3_distance(myBullets[i].position,tree_collider[t].position) < 0.2+1){
					myBullets[i].active = 0;
					break;
				}
			}
		}
	}
}

void collide_my_bullets(){
	for(int i =0 ; i < 5; i ++){
		// collide with trees
		if(myBullets[i].active == 0)continue;
		for(int t = 0; t < 11; t++){
			if(glms_vec3_distance(myBullets[i].position,tree_collider[t].position) < 0.2+1){
				myBullets[i].active = 0;
				break;
			}
		}

		// collide with other bullets
		if(myBullets[i].active == 0)continue;
		for(int j =0 ; j < 10; j++){
			if(bullets[j].active == 0)continue;
			if(glm_vec3_distance(bullets[i].position.raw,myBullets[i].position.raw) < 0.2+0.2){
				bullets[j].active = 0;
				myBullets[i].active = 0;
				break;
			}
		}
		//collid with enemgy
		for(int j =0 ; j < 10; j ++){
			if(myBullets[i].active == 0)continue;
			if(glm_vec3_distance(guns[j],myBullets[i].position.raw) < 1+0.2){
				guns_bested[j] -= 35;
				myBullets[i].active = 0;
				break;
			}
	}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		int i = 0;
		do{i++;}while(myBullets[i].active && i < 5);
		if(i == 5)i=0;
		printf("\n %i",i);
		vec3s front;
		front.x = camera.front.x;
		front.y = 0;
		front.z = camera.front.z;
		front = glms_vec3_normalize(front);
		myBullets[i].position = position;
		myBullets[i].direction = front;
		myBullets[i].active  = 1;
		
	}
}
int main()
{
	init_glfw();
	GLFWwindow* window = create_glfw_window();
	init_glad();
	gl_settings();
	unsigned int shaderProgram;
	{
		unsigned int vertexShader  = compile_shader("engine/shaders/env.vs",GL_VERTEX_SHADER);
		unsigned int fragShader  = compile_shader("engine/shaders/env.fs",GL_FRAGMENT_SHADER);
		shaderProgram = create_program(vertexShader,fragShader);
		set_shader_texture_units(shaderProgram);
	}
	unsigned int instancedShaderProgram;
	{
		unsigned int vertexShader  = compile_shader("engine/shaders/env_instanced.vs",GL_VERTEX_SHADER);
		unsigned int fragShader  = compile_shader("engine/shaders/env.fs",GL_FRAGMENT_SHADER);
		instancedShaderProgram = create_program(vertexShader,fragShader);
		set_shader_texture_units(instancedShaderProgram);
	}
	unsigned int Hshader;
	{
		unsigned int vertexShader = compile_shader("engine/shaders/hv.txt", GL_VERTEX_SHADER);
		unsigned int fragShader = compile_shader("engine/shaders/hf.txt", GL_FRAGMENT_SHADER);
		Hshader = create_program(vertexShader, fragShader);
		//set_shader_texture_units(Hshader);
	}
	GLuint vao = setup_debug_cube();

	// set uniforms
	{
		mat4s projection = glms_perspective(glm_rad(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		mat4s cube_transform = GLMS_MAT4_IDENTITY_INIT;
		setUniformMat4s(shaderProgram,projection,"projection");
		setUniformMat4s(shaderProgram,cube_transform,"model");
		setUniformMat4s(instancedShaderProgram,projection,"projection");
		setUniformMat4s(instancedShaderProgram,cube_transform,"model");
	}
	
	primitive_env sky;
	primitive_env gun;
	primitive_env bullet;
	
	load_model_env("models/gun.gltf",&gun,0);
	load_model_env("models/anthill.gltf",&anthill,0);
	//load_model_env("models/tree.glb", tree, 0);
	load_model_env("models/leaf.glb", &bark, 0);
	load_model_env("models/bark.glb", &leaf, 0);
	load_model_env("models/sky.glb",&sky,0);
	load_model_env("models/grass.gltf",&grass,0);
	load_model_env("models/pango_ball_1.glb",&pango_ball_model,0);
	load_model_env("models/pango.gltf",&pango_model,0);
	tree_m[0] =  upload_single_primitive_env(bark);
	tree_m[1] =  upload_single_primitive_env(leaf);
	drawable_mesh gun_m =  upload_single_primitive_env(gun);
	drawable_mesh sky_m =  upload_single_primitive_env(sky);
	drawable_mesh grass_m =  upload_single_primitive_env(grass);
	drawable_mesh pango_b_m =  upload_single_primitive_env(pango_ball_model);
	drawable_mesh pango_m =  upload_single_primitive_env(pango_model);
	drawable_mesh anthill_m =  upload_single_primitive_env(anthill);

	generate_trees(10);
	//primitive_count_env = load_model_env("models/simple_skin.gltf",primitives_env,0);
	//unsigned int env_vao =  upload_multiple_primitive_env(primitives_env,primitive_count_env);


	init_camera();

	position.y = pRadius;
	float obspos[3] = {8,0,8};
	Sphere sphere = generateSphere(1,50);
	Sphere bullet_sphere = generateSphere(0.2,5);
	int next_shoot = 10;
	int frequency = 4;
    while (!glfwWindowShouldClose(window))
    {
		// game stuff
		update_delta_time();
		update_third_person_camera();
		processInput(window);
		for(int i = 0 ;  i < 11; i++){
			position = collide(position,pRadius,tree_collider[i]);
		}
		check_ant_hill_collision(position,pRadius);
		calc_translation();

		if(health == 0) exit(1);
		
		
		glm_lookat(camera.position.raw,position.raw,camera.up.raw,camera.lookAt.raw);
		setUniformMat4s(shaderProgram,camera.lookAt,"view");
		setUniformMat4s(instancedShaderProgram,camera.lookAt,"view");

		// shoot bullets 
		if(glfwGetTime() > next_shoot){
			shoot_bullets();
			next_shoot = glfwGetTime() + 5;
		}
		//travel_bullets();
		for(int i = 0 ; i < 10; i++){
			if(guns_bested[i] > 0){
				bullets[i].position = glms_vec3_sub(bullets[i].position,glms_vec3_scale(bullets[i].direction, 20 * delta_time));
			}
		}
		// travel muy Bullets
		for(int i = 0 ; i < 5; i++){
			if(!myBullets[i].active)continue;
			myBullets[i].position = glms_vec3_sub(myBullets[i].position,glms_vec3_scale(myBullets[i].direction, 20 * delta_time));
		}
		collide_bullets(position,pRadius);
		collide_my_bullets();
        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//draw_multiple_primitive_env(shaderProgram,env_vao, primitives_env,primitive_count_env);

		// draw pango
		{
			mat4s cube_transform = GLMS_MAT4_IDENTITY_INIT;
			cube_transform = glms_translate(cube_transform,position);
			if(!mode){
				cube_transform = glms_rotate(cube_transform,glm_rad(-angle),camera.right);
				cube_transform = glms_rotate(cube_transform,glm_rad(-yaw),up);
			}else{
				cube_transform = glms_rotate(cube_transform,glm_rad(180-yaw),up);
			}
			cube_transform = glms_scale_uni(cube_transform,pRadius);
			setUniformMat4s(shaderProgram,cube_transform,"model");
			if(!mode){
				draw_single_primitive_env(shaderProgram,pango_b_m);
			}else{
				draw_single_primitive_env(shaderProgram,pango_m);
			}
		}

		// draw grass
		{
			mat4s grass = GLMS_MAT4_IDENTITY_INIT;
			setUniformMat4s(shaderProgram,grass,"model");
			draw_single_primitive_env(shaderProgram,grass_m);
		}

		// draw bushes
		{

		}

		// lets add trees
		draw_trees(instancedShaderProgram,11);

		// draw anthills
		for(int i = 0; i < 3; i++){
			mat4 cube_transform = GLM_MAT4_IDENTITY_INIT;
			glm_translate(cube_transform,anthill_location[i]);
			setUniformMat4(shaderProgram,cube_transform,"model");
			draw_single_primitive_env(shaderProgram,anthill_m);
		}

		// draw guns

		bool once =0;
		for(int i =0 ; i < 10; i++){
			
			if(guns_bested[i] > 0){
				once = 1;
				mat4 cube_transform = GLM_MAT4_IDENTITY_INIT;
				glm_translate(cube_transform,guns[i]);
				setUniformMat4(shaderProgram,cube_transform,"model");
				draw_single_primitive_env(shaderProgram,gun_m);
			}
		}
		if (!once)exit(0);

		// draw bullets
		{
			for(int i = 0; i < 10; i++){
				if(bullets[i].active == 0) continue;
				mat4 cube_transform = GLM_MAT4_IDENTITY_INIT;
				glm_translate(cube_transform,bullets[i].position.raw);
				//glm_scale_uni(cube_transform,40);
				setUniformMat4(shaderProgram,cube_transform,"model");
				glUseProgram(shaderProgram);
				drawSphere(bullet_sphere);
			}
		}
		// draw mybullets
		{
			for(int i = 0; i < 5; i++){
				if(myBullets[i].active == 0) continue;
				mat4 cube_transform = GLM_MAT4_IDENTITY_INIT;
				glm_translate(cube_transform,myBullets[i].position.raw);
				setUniformMat4(shaderProgram,cube_transform,"model");
				glUseProgram(shaderProgram);
				drawSphere(bullet_sphere);
			}
		}
		// lets draw the skybox at the end
		{
			mat4s obstacleT = GLMS_MAT4_IDENTITY_INIT;
			setUniformMat4s(shaderProgram,obstacleT,"model");
			draw_single_primitive_env(shaderProgram,sky_m);
			
		}
		glDisable(GL_DEPTH_TEST);
		drawHealthBar(health / 100.0, -1900.0 / 1920, -1060.0 / 1080, 900.0 / 1920, 100.0 / 1080, Hshader);

		glEnable(GL_DEPTH_TEST);
		//glBindVertexArray(vao);
		//setUniformMat4s(shaderProgram,og,"model");
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
	printf("\nbye!\n");
    return 0;
}
