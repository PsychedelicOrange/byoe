#version 460 core

in vec3 fragPos;
in vec3 normal_vs;
in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D base_color;
uniform sampler2D normal;
uniform sampler2D rough_metal;

void main()
{
	vec4 color_raw = texture(base_color, texCoord);
	FragColor = color_raw * 0.5f;
}
