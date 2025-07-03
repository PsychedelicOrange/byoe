Texture2D<float4> sceneTexture : register(t0, space0);
SamplerState sceneSampler : register(s1, space0);

static float4 outColorRenderTarget;
static float2 inUV;

struct SPIRV_Cross_Input
{
    float2 inUV : TEXCOORD0;
};

struct SPIRV_Cross_Output
{
    float4 outColorRenderTarget : SV_Target0;
};

void frag_main()
{
    outColorRenderTarget = sceneTexture.Sample(sceneSampler, inUV);
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    inUV = stage_input.inUV;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.outColorRenderTarget = outColorRenderTarget;
    return stage_output;
}
