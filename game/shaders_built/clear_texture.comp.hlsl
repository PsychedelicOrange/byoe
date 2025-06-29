static const uint3 gl_WorkGroupSize = uint3(8u, 8u, 1u);

RWTexture2D<float4> outColorRenderTarget : register(u0, space0);

static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

void comp_main()
{
    outColorRenderTarget[int2(gl_GlobalInvocationID.xy)] = 0.0f.xxxx;
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
