static float4 gl_Position;
static int gl_VertexIndex;
static float2 outUV;

struct SPIRV_Cross_Input
{
    uint gl_VertexIndex : SV_VertexID;
};

struct SPIRV_Cross_Output
{
    float2 outUV : TEXCOORD0;
    float4 gl_Position : SV_Position;
};

void vert_main()
{
    float x = float((gl_VertexIndex << 1) & 2);
    float y = float(gl_VertexIndex & 2);
    float2 uv = float2(x, y);
    outUV = uv;
    gl_Position = float4((uv * 2.0f) - 1.0f.xx, 0.0f, 1.0f);
    gl_Position.y = -gl_Position.y;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    gl_VertexIndex = int(stage_input.gl_VertexIndex);
    vert_main();
    SPIRV_Cross_Output stage_output;
    stage_output.gl_Position = gl_Position;
    stage_output.outUV = outUV;
    return stage_output;
}
