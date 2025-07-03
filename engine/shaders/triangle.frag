#version 450

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColorRenderTarget;

void main() {
    outColorRenderTarget = float4(inColor, 1.0f);
}
	
