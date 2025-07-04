// #version 450
// 
// layout(location = 0) in vec2 inUV;
// 
// layout(location = 0) out vec4 outColorRenderTarget;
// 
// layout(binding = 0, set = 0) uniform sampler2D sceneTexture;
// 
// void main() {
//     outColorRenderTarget = texture(sceneTexture, inUV);
// }

////////////////////////////////////////////////////////////////////////	

#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColorRenderTarget;

layout(binding = 0, set = 0) uniform texture2D sceneTexture;
layout(binding = 1, set = 0) uniform sampler sceneSampler;

void main() {
    outColorRenderTarget = texture(sampler2D(sceneTexture, sceneSampler), inUV);
}
	