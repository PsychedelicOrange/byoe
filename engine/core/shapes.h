
typedef struct Sphere{
	float radius;
	unsigned int vao;
	float numIndices;
}Sphere;

Sphere generateSphere(float radius, int subdivisions);
unsigned int setup_debug_cube();
void drawSphere(Sphere sphere);

