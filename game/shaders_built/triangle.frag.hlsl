static float4 outColorRenderTarget;
static float3 inColor;

struct SPIRV_Cross_Input
{
    float3 inColor : TEXCOORD0;
};

struct SPIRV_Cross_Output
{
    float4 outColorRenderTarget : SV_Target0;
};

void frag_main()
{
    outColorRenderTarget = float4(inColor, 1.0f);
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    inColor = stage_input.inColor;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.outColorRenderTarget = outColorRenderTarget;
    return stage_output;
}
