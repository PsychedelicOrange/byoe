#ifndef TRANSFORM_H
#define TRANSFORM_H

typedef struct Transform
{
    vec3   position;
    float  _padding1;
    versor rotation;    // glm::quat [x, y, z, w]
    vec3   scale;
    float  _padding2;
} Transform;

#endif