struct SDF_Material
{
    float4 diffuse;
};

struct hit_info
{
    float d;
    SDF_Material material;
};

struct Ray
{
    float3 ro;
    float3 rd;
};

struct SDF_Node
{
    int nodeType;
    int primType;
    row_major float4x4 transform;
    float scale;
    float4 packed_params[2];
    int blend;
    int prim_a;
    int prim_b;
    SDF_Material material;
};

struct blend_node
{
    int blend;
    int node;
    float4x4 transform;
};

static const uint3 gl_WorkGroupSize = uint3(8u, 8u, 1u);

cbuffer SDFScene : register(b0, space0)
{
    SDF_Node _1341_nodes[100] : packoffset(c0);
};

cbuffer PushConstant
{
    row_major float4x4 pc_data_view_proj : packoffset(c0);
    int2 pc_data_resolution : packoffset(c4);
    float3 pc_data_dir_light_pos : packoffset(c5);
    int pc_data_curr_draw_node_idx : packoffset(c5.w);
};

RWTexture2D<float4> outColorRenderTarget : register(u1, space0);

static uint3 gl_GlobalInvocationID;
struct SPIRV_Cross_Input
{
    uint3 gl_GlobalInvocationID : SV_DispatchThreadID;
};

void comp_main()
{
    float2 uv = float2(gl_GlobalInvocationID.xy) / float2(pc_data_resolution);
    float2 ndcPos = ((uv * 2.0f) - 1.0f.xx) * float2(1.0f, -1.0f);
    outColorRenderTarget[int2(gl_GlobalInvocationID.xy)] = float4(ndcPos, 1.0f, 1.0f);
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
