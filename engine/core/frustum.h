#ifndef FRUSTUM_H
#define FRUSTUM_H
#include <stddef.h>
#include <cglm/cglm.h>   /* for inline */

// TODO: From a clean code perspectice I'm not a huge fan of output variables as func args so refactor this function
int cull_nodes(int count, vec4* nodes, vec4* out_visible_nodes, mat4 viewproj);
#endif
