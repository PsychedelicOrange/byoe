#ifndef CAMERA_H
#define CAMERA_H

typedef struct Camera
{
    mat4s lookAt;
    vec3s position;
    vec3s right;
    vec3s front;
    vec3s up;
    float near_plane;
    float far_plane;
    float fov;
    float yaw;
    float pitch;
} Camera;

#endif