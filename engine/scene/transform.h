#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "../core/common.h"

#ifndef SHADER_INCLUDE
    #include <cglm/cglm.h>
    #include <cglm/struct.h>
#endif

STRUCT(Transform,
    vec3s  position;
    float  _padding1;
    versor rotation;    // glm::quat [x, y, z, w]
    vec3s  scale;
    float  _padding2;
)

#endif