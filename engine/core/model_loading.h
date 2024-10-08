#ifndef MODEL_LOADING
#define MODEL_LOADING

#include <stddef.h>
#include "cgltf.h"
#include "mesh.h"
#include "utils.h"



void bind_textures(material_env mat);
void load_primitives_env (cgltf_mesh* mesh, primitive_env* primitives, size_t primitive_index);
size_t load_model_env(char* model_path, primitive_env* primitives, size_t primitive_index);
void draw_multiple_primitive_env(unsigned int shaderProgram,unsigned int env_vao,primitive_env* primitives, size_t primitive_count);
void draw_single_primitive_env(unsigned int shaderProgram,drawable_mesh d);
void vertexAttrib_env();
drawable_mesh upload_single_primitive_env(primitive_env p);
buffer append_primitive_vertice_data(primitive_env* primitives,size_t from,size_t to);
buffer append_primitive_indice_data(primitive_env* primitives,size_t from,size_t to);
unsigned int upload_multiple_primitive_env(primitive_env* primitives, size_t primitive_count);

#endif
