// add ifnde
//
#include <stddef.h>


typedef struct material_env{
	unsigned int base_color;
	unsigned int rough_metal;
	unsigned int normal;
}material_env;

typedef struct drawable_mesh{
	unsigned int vao;
	size_t indices_count;
	material_env mat;
}drawable_mesh;

typedef struct vertex_env{
	float position[3];
	float normal[3];
	float tangent[4];
	float uv[2];
	float uv1[2];
}vertex_env;

typedef struct primitive_env{
	vertex_env* vertices;
	size_t vertex_count;
	unsigned short* indices;
	size_t indices_count;
	// index into large buffer
	size_t base_vertex;
	size_t index_index; // lol

	material_env material;
}primitive_env;

