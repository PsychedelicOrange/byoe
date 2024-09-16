#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aTangent;
layout (location = 3) in vec2 aTexcoords;
layout (location = 4) in vec2 aTexcoords1;

out vec3 fragPos;
out vec3 normal;

void main()
{
	normal = aNormal;
	gl_Position = vec4(aPos,1.0f);
	fragPos = vec3(aPos.x,aPos.y,aPos.z);
}
