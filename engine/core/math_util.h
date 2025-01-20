#ifndef MATH_UTIL_H
#define MATH_UTIL_H

#include <cglm/struct.h>

mat4s create_transform_matrix(vec3 position, versor rotation, vec3 scale);

#endif