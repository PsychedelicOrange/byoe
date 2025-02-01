#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColorRenderTarget;

layout(binding = 0, set = 0, rgba32f) readonly uniform image2D sceneTexture;

void main() {
    vec4 color = imageLoad(sceneTexture, ivec2(inUV * imageSize(sceneTexture)));
    outColorRenderTarget = color;
}
	
