#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColorRenderTarget;

//layout(binding = 0, set = 0) uniform sampler2D sceneTexture;

void main() {
    outColorRenderTarget = vec4(inUV, 0.0f, 1.0f);
}
	
