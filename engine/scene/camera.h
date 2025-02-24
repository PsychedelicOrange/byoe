#ifndef CAMERA_H
#define CAMERA_H

typedef struct Camera
{
    mat4s lookAt;
    vec3s position;
    float near_plane;
    vec3s right;
    float far_plane;
    vec3s front;
    float fov;
    vec3s up;
    float yaw;
    float pitch;
    float _pad0[3];
} Camera;

#endif