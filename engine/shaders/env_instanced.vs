#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aTangent;
layout (location = 3) in vec2 aTexcoords;
layout (location = 4) in vec2 aTexcoords1;

layout(std430, binding = 1) buffer modelMatrices
{
    mat4 models[];
};


out vec3 fragPos;
out vec3 normal_v;
out vec2 texCoord;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	normal_v = aNormal;
	gl_Position =  projection * view * models[gl_InstanceID] * vec4(aPos,1.0f);
	texCoord = aTexcoords;
	fragPos = vec3(aPos.x,aPos.y,aPos.z);
}
