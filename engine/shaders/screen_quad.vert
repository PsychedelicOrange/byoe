#version 450

layout(location = 0) out vec2 outUV;

out gl_PerVertex
{
    vec4 gl_Position;
};
//------------------------------------------------------------------------------
void main()
{
    // Calculate UV coordinates based on gl_VertexID
    float x = float((gl_VertexIndex << 1) & 2);
    float y = float(gl_VertexIndex & 2);
    vec2 uv = vec2(x, y);
    outUV = uv;
    
    // Set the final vertex position
    gl_Position = vec4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
}