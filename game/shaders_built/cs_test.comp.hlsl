static const uint3 gl_WorkGroupSize = uint3(32u, 32u, 1u);

cbuffer CSTestUBO : register(b1, space0)
{
    float4 ubo_color : packoffset(c0);
};

cbuffer PushConstant
{
    row_major float4x4 pc_data_view_proj : packoffset(c0);
    int2 pc_data_resolution : packoffset(c4);
    float3 pc_data_dir_light_pos : packoffset(c5);
    int pc_data_curr_draw_node_idx : packoffset(c5.w);
};

RWTexture2D<float4> outColorRenderTarget : register(u0, space0);

static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

void comp_main()
{
    float2 uv = float2(gl_GlobalInvocationID.xy) / float2(800.0f, 600.0f);
    outColorRenderTarget[int2(gl_GlobalInvocationID.xy)] = ubo_color;
}

[numthreads(32, 32, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
