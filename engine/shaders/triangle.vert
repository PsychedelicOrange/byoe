#version 450

layout(location = 0) out vec3 outColor;

out gl_PerVertex
{
    vec4 gl_Position;
};
//------------------------------------------------------------------------------
void main()
{
    vec2 positions[3] = vec2[](
        vec2( 0.0,  -0.5),  // top
        vec2(-0.5, 0.5),  // bottom-left
        vec2( 0.5, 0.5)   // bottom-right
    );

    vec3 colors[3] = vec3[](
        vec3(1.0, 0.0, 0.0),  // red
        vec3(0.0, 1.0, 0.0),  // green
        vec3(0.0, 0.0, 1.0)   // blue
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outColor = colors[gl_VertexIndex];
}