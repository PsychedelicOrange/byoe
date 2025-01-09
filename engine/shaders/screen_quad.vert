#version 410 core
layout (location = 0) in vec2 aPos;

uniform mat4 viewproj;
out vec4 nearp;
out vec4 farp;

void main()
{
	vec4 position = vec4(aPos,0,1);
	gl_Position = position;
	vec2 pos = gl_Position.xy/gl_Position.w;

	nearp = inverse(viewproj) * vec4(pos,-1,1);
	farp = inverse(viewproj) * vec4(pos,1,1);
}
	
