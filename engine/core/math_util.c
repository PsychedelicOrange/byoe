#include "math_util.h"

mat4s create_transform_matrix(vec3 position, versor rotation, vec3 scale)
{
    mat4s translation    = {0};
    mat4s rotationMatrix = {0};
    mat4s scaling        = {0};
    mat4s result         = {0};

    glm_translate_make(translation.raw, position);

    glm_mat4_identity(rotationMatrix.raw);
    glm_quat_rotate(translation.raw, rotation, rotationMatrix.raw);

    glm_scale_make(scaling.raw, scale);

    glm_mat4_mul(translation.raw, rotationMatrix.raw, result.raw);
    glm_mat4_mul(result.raw, scaling.raw, result.raw);

    return result;
}