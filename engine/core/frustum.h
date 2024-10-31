#ifndef FRUSTUM_H
#define FRUSTUM_H
#include <stddef.h>
#include <cglm/cglm.h>   /* for inline */
struct camera_details{
	float aspect;
	float fov_rad;
	float near;
	float far;
};
int cull_rocks (int count,vec4* rocks,vec4* rocks_visible, mat4 viewproj);
#endif
