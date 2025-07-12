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

// Returns the determinant of a 2x2 matrix.
float spvDet2x2(float a1, float a2, float b1, float b2)
{
    return a1 * b2 - b1 * a2;
}

// Returns the determinant of a 3x3 matrix.
float spvDet3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3)
{
    return a1 * spvDet2x2(b2, b3, c2, c3) - b1 * spvDet2x2(a2, a3, c2, c3) + c1 * spvDet2x2(a2, a3, b2, b3);
}

// Returns the inverse of a matrix, by using the algorithm of calculating the classical
// adjoint and dividing by the determinant. The contents of the matrix are changed.
float4x4 spvInverse(float4x4 m)
{
    float4x4 adj;	// The adjoint matrix (inverse after dividing by determinant)

    // Create the transpose of the cofactors, as the classical adjoint of the matrix.
    adj[0][0] =  spvDet3x3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
    adj[0][1] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
    adj[0][2] =  spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[3][1], m[3][2], m[3][3]);
    adj[0][3] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3]);

    adj[1][0] = -spvDet3x3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
    adj[1][1] =  spvDet3x3(m[0][0], m[0][2], m[0][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
    adj[1][2] = -spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[3][0], m[3][2], m[3][3]);
    adj[1][3] =  spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3]);

    adj[2][0] =  spvDet3x3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
    adj[2][1] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
    adj[2][2] =  spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[3][0], m[3][1], m[3][3]);
    adj[2][3] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3]);

    adj[3][0] = -spvDet3x3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
    adj[3][1] =  spvDet3x3(m[0][0], m[0][1], m[0][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
    adj[3][2] = -spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[3][0], m[3][1], m[3][2]);
    adj[3][3] =  spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2]);

    // Calculate the determinant as a combination of the cofactors of the first row.
    float det = (adj[0][0] * m[0][0]) + (adj[0][1] * m[1][0]) + (adj[0][2] * m[2][0]) + (adj[0][3] * m[3][0]);

    // Divide the classical adjoint matrix by the determinant.
    // If determinant is zero, matrix is not invertable, so leave it unchanged.
    return (det != 0.0f) ? (adj * (1.0f / det)) : m;
}

float3 opTx(float3 p, float4x4 t)
{
    return mul(float4(p, 1.0f), spvInverse(t)).xyz;
}

float Sphere_get_radius(float4 packed1, float4 packed2)
{
    return packed1.x;
}

float sphereSDF(float3 p, float radius)
{
    return length(p) - radius;
}

float3 Box_get_dimensions(float4 packed1, float4 packed2)
{
    return packed1.xyz;
}

float boxSDF(float3 p, float3 size)
{
    float3 q = abs(p) - size;
    return length(max(q, 0.0f.xxx)) + min(max(q.x, max(q.y, q.z)), 0.0f);
}

float3 RoundBox_get_dimensions(float4 packed1, float4 packed2)
{
    return packed1.xyz;
}

float RoundBox_get_roundness(float4 packed1, float4 packed2)
{
    return packed1.w;
}

float roundBoxSDF(float3 p, float3 b, float r)
{
    float3 q = (abs(p) - b) + r.xxx;
    return (length(max(q, 0.0f.xxx)) + min(max(q.x, max(q.y, q.z)), 0.0f)) - r;
}

float3 BoxFrame_get_dimensions(float4 packed1, float4 packed2)
{
    return packed1.xyz;
}

float BoxFrame_get_thickness(float4 packed1, float4 packed2)
{
    return packed1.w;
}

float boxFrameSDF(inout float3 p, float3 b, float e)
{
    p = abs(p) - b;
    float3 q = abs(p + e.xxx) - e.xxx;
    return min(min(length(max(float3(p.x, q.y, q.z), 0.0f.xxx)) + min(max(p.x, max(q.y, q.z)), 0.0f), length(max(float3(q.x, p.y, q.z), 0.0f.xxx)) + min(max(q.x, max(p.y, q.z)), 0.0f)), length(max(float3(q.x, q.y, p.z), 0.0f.xxx)) + min(max(q.x, max(q.y, p.z)), 0.0f));
}

float2 Torus_get_thickness(float4 packed1, float4 packed2)
{
    return packed1.xy;
}

float torusSDF(float3 p, float2 t)
{
    float2 q = float2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

float2 CappedTorus_get_majorMinor(float4 packed1, float4 packed2)
{
    return packed1.xy;
}

float CappedTorus_get_radiusA(float4 packed1, float4 packed2)
{
    return packed1.z;
}

float CappedTorus_get_radiusB(float4 packed1, float4 packed2)
{
    return packed1.w;
}

float cappedTorusSDF(inout float3 p, float2 sc, float ra, float rb)
{
    p.x = abs(p.x);
    float _600;
    if ((sc.y * p.x) > (sc.x * p.y))
    {
        _600 = dot(p.xy, sc);
    }
    else
    {
        _600 = length(p.xy);
    }
    float k = _600;
    return sqrt((dot(p, p) + (ra * ra)) - ((2.0f * ra) * k)) - rb;
}

float3 Capsule_get_start(float4 packed1, float4 packed2)
{
    return packed1.xyz;
}

float3 Capsule_get_end(float4 packed1, float4 packed2)
{
    return float3(packed1.w, packed2.x, packed2.y);
}

float Capsule_get_radius(float4 packed1, float4 packed2)
{
    return packed2.z;
}

float capsuleSDF(float3 p, float3 a, float3 b, float r)
{
    float3 pa = p - a;
    float3 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0f, 1.0f);
    return length(pa - (ba * h)) - r;
}

float VerticalCapsule_get_radius(float4 packed1, float4 packed2)
{
    return packed1.x;
}

float VerticalCapsule_get_height(float4 packed1, float4 packed2)
{
    return packed1.y;
}

float verticalCapsuleSDF(inout float3 p, float r, float h)
{
    p.y -= clamp(p.y, 0.0f, h);
    return length(p) - r;
}

float Cylinder_get_radius(float4 packed1, float4 packed2)
{
    return packed1.x;
}

float Cylinder_get_height(float4 packed1, float4 packed2)
{
    return packed1.y;
}

float cappedCylinderSDF(float3 p, float h, float r)
{
    float2 d = abs(float2(length(p.xz), p.y)) - float2(r, h);
    return min(max(d.x, d.y), 0.0f) + length(max(d, 0.0f.xx));
}

float RoundedCylinder_get_radiusA(float4 packed1, float4 packed2)
{
    return packed1.x;
}

float RoundedCylinder_get_radiusB(float4 packed1, float4 packed2)
{
    return packed1.y;
}

float RoundedCylinder_get_height(float4 packed1, float4 packed2)
{
    return packed1.z;
}

float roundedCylinderSDF(float3 p, float ra, float rb, float h)
{
    float2 d = float2((length(p.xz) - (2.0f * ra)) + rb, abs(p.y) - h);
    return (min(max(d.x, d.y), 0.0f) + length(max(d, 0.0f.xx))) - rb;
}

float3 Ellipsoid_get_radii(float4 packed1, float4 packed2)
{
    return packed1.xyz;
}

float ellipsoidSDF(float3 p, float3 r)
{
    float k0 = length(p / r);
    float k1 = length(p / (r * r));
    return (k0 * (k0 - 1.0f)) / k1;
}

float2 HexagonalPrism_get_radius(float4 packed1, float4 packed2)
{
    return packed1.xy;
}

float hexPrismSDF(inout float3 p, float2 h)
{
    p = abs(p);
    float3 _752 = p;
    float3 _758 = p;
    float2 _760 = _758.xy - (float2(-0.866025388240814208984375f, 0.5f) * (2.0f * min(dot(float2(-0.866025388240814208984375f, 0.5f), _752.xy), 0.0f)));
    p.x = _760.x;
    p.y = _760.y;
    float2 d = float2(length(p.xy - float2(clamp(p.x, (-0.57735002040863037109375f) * h.x, 0.57735002040863037109375f * h.x), h.x)) * sign(p.y - h.x), p.z - h.y);
    return min(max(d.x, d.y), 0.0f) + length(max(d, 0.0f.xx));
}

float TriangularPrism_get_radius(float4 packed1, float4 packed2)
{
    return packed1.x;
}

float TriangularPrism_get_height(float4 packed1, float4 packed2)
{
    return packed1.y;
}

float triPrismSDF(float3 p, float2 h)
{
    float3 q = abs(p);
    return max(q.z - h.y, max((q.x * 0.86602497100830078125f) + (p.y * 0.5f), -p.y) - (h.x * 0.5f));
}

float Cone_get_angle(float4 packed1, float4 packed2)
{
    return packed1.x;
}

float Cone_get_height(float4 packed1, float4 packed2)
{
    return packed1.y;
}

float coneSDF(float3 p, float angle, float h)
{
    float q = length(p.xz);
    float2 c = float2(sin(angle), cos(angle));
    return max(dot(c, float2(q, p.y)), (-h) - p.y);
}

float CappedCone_get_radiusTop(float4 packed1, float4 packed2)
{
    return packed1.x;
}

float CappedCone_get_radiusBottom(float4 packed1, float4 packed2)
{
    return packed1.y;
}

float CappedCone_get_height(float4 packed1, float4 packed2)
{
    return packed1.z;
}

float dot2(float2 v)
{
    return dot(v, v);
}

float cappedConeSDF(float3 p, float h, float r1, float r2)
{
    float2 q = float2(length(p.xz), p.y);
    float2 k1 = float2(r2, h);
    float2 k2 = float2(r2 - r1, 2.0f * h);
    float2 ca = float2(q.x - min(q.x, (q.y < 0.0f) ? r1 : r2), abs(q.y) - h);
    float2 param = k2;
    float2 cb = (q - k1) + (k2 * clamp(dot(k1 - q, k2) / dot2(param), 0.0f, 1.0f));
    bool _918 = cb.x < 0.0f;
    bool _924;
    if (_918)
    {
        _924 = ca.y < 0.0f;
    }
    else
    {
        _924 = _918;
    }
    float s = _924 ? (-1.0f) : 1.0f;
    float2 param_1 = ca;
    float2 param_2 = cb;
    return s * sqrt(min(dot2(param_1), dot2(param_2)));
}

float3 Plane_get_normal(float4 packed1, float4 packed2)
{
    return packed1.xyz;
}

float Plane_get_distance(float4 packed1, float4 packed2)
{
    return packed1.w;
}

float planeSDF(float3 p, float3 n, float h)
{
    return dot(p, n) + h;
}

float getPrimitiveSDF(float3 local_p, int primType, float4 packed1, float4 packed2)
{
    float d = 0.0f;
    switch (primType)
    {
        case 0:
        {
            float4 param = packed1;
            float4 param_1 = packed2;
            float3 param_2 = local_p;
            float param_3 = Sphere_get_radius(param, param_1);
            d = sphereSDF(param_2, param_3);
            break;
        }
        case 1:
        {
            float4 param_4 = packed1;
            float4 param_5 = packed2;
            float3 param_6 = local_p;
            float3 param_7 = Box_get_dimensions(param_4, param_5);
            d = boxSDF(param_6, param_7);
            break;
        }
        case 2:
        {
            float4 param_8 = packed1;
            float4 param_9 = packed2;
            float4 param_10 = packed1;
            float4 param_11 = packed2;
            float3 param_12 = local_p;
            float3 param_13 = RoundBox_get_dimensions(param_8, param_9);
            float param_14 = RoundBox_get_roundness(param_10, param_11);
            d = roundBoxSDF(param_12, param_13, param_14);
            break;
        }
        case 3:
        {
            float4 param_15 = packed1;
            float4 param_16 = packed2;
            float4 param_17 = packed1;
            float4 param_18 = packed2;
            float3 param_19 = local_p;
            float3 param_20 = BoxFrame_get_dimensions(param_15, param_16);
            float param_21 = BoxFrame_get_thickness(param_17, param_18);
            float _1120 = boxFrameSDF(param_19, param_20, param_21);
            d = _1120;
            break;
        }
        case 4:
        {
            float4 param_22 = packed1;
            float4 param_23 = packed2;
            float3 param_24 = local_p;
            float2 param_25 = Torus_get_thickness(param_22, param_23);
            d = torusSDF(param_24, param_25);
            break;
        }
        case 5:
        {
            float4 param_26 = packed1;
            float4 param_27 = packed2;
            float4 param_28 = packed1;
            float4 param_29 = packed2;
            float4 param_30 = packed1;
            float4 param_31 = packed2;
            float3 param_32 = local_p;
            float2 param_33 = CappedTorus_get_majorMinor(param_26, param_27);
            float param_34 = CappedTorus_get_radiusA(param_28, param_29);
            float param_35 = CappedTorus_get_radiusB(param_30, param_31);
            float _1152 = cappedTorusSDF(param_32, param_33, param_34, param_35);
            d = _1152;
            break;
        }
        case 6:
        {
            float4 param_36 = packed1;
            float4 param_37 = packed2;
            float4 param_38 = packed1;
            float4 param_39 = packed2;
            float4 param_40 = packed1;
            float4 param_41 = packed2;
            float3 param_42 = local_p;
            float3 param_43 = Capsule_get_start(param_36, param_37);
            float3 param_44 = Capsule_get_end(param_38, param_39);
            float param_45 = Capsule_get_radius(param_40, param_41);
            d = capsuleSDF(param_42, param_43, param_44, param_45);
            break;
        }
        case 7:
        {
            float4 param_46 = packed1;
            float4 param_47 = packed2;
            float4 param_48 = packed1;
            float4 param_49 = packed2;
            float3 param_50 = local_p;
            float param_51 = VerticalCapsule_get_radius(param_46, param_47);
            float param_52 = VerticalCapsule_get_height(param_48, param_49);
            float _1190 = verticalCapsuleSDF(param_50, param_51, param_52);
            d = _1190;
            break;
        }
        case 8:
        {
            float4 param_53 = packed1;
            float4 param_54 = packed2;
            float4 param_55 = packed1;
            float4 param_56 = packed2;
            float3 param_57 = local_p;
            float param_58 = Cylinder_get_radius(param_53, param_54);
            float param_59 = Cylinder_get_height(param_55, param_56);
            d = cappedCylinderSDF(param_57, param_58, param_59);
            break;
        }
        case 9:
        {
            float4 param_60 = packed1;
            float4 param_61 = packed2;
            float4 param_62 = packed1;
            float4 param_63 = packed2;
            float4 param_64 = packed1;
            float4 param_65 = packed2;
            float3 param_66 = local_p;
            float param_67 = RoundedCylinder_get_radiusA(param_60, param_61);
            float param_68 = RoundedCylinder_get_radiusB(param_62, param_63);
            float param_69 = RoundedCylinder_get_height(param_64, param_65);
            d = roundedCylinderSDF(param_66, param_67, param_68, param_69);
            break;
        }
        case 10:
        {
            float4 param_70 = packed1;
            float4 param_71 = packed2;
            float3 param_72 = local_p;
            float3 param_73 = Ellipsoid_get_radii(param_70, param_71);
            d = ellipsoidSDF(param_72, param_73);
            break;
        }
        case 11:
        {
            float4 param_74 = packed1;
            float4 param_75 = packed2;
            float3 param_76 = local_p;
            float2 param_77 = HexagonalPrism_get_radius(param_74, param_75);
            float _1248 = hexPrismSDF(param_76, param_77);
            d = _1248;
            break;
        }
        case 12:
        {
            float4 param_78 = packed1;
            float4 param_79 = packed2;
            float4 param_80 = packed1;
            float4 param_81 = packed2;
            float3 param_82 = local_p;
            float2 param_83 = float2(TriangularPrism_get_radius(param_78, param_79), TriangularPrism_get_height(param_80, param_81));
            d = triPrismSDF(param_82, param_83);
            break;
        }
        case 13:
        {
            float4 param_84 = packed1;
            float4 param_85 = packed2;
            float4 param_86 = packed1;
            float4 param_87 = packed2;
            float3 param_88 = local_p;
            float param_89 = Cone_get_angle(param_84, param_85);
            float param_90 = Cone_get_height(param_86, param_87);
            d = coneSDF(param_88, param_89, param_90);
            break;
        }
        case 14:
        {
            float4 param_91 = packed1;
            float4 param_92 = packed2;
            float4 param_93 = packed1;
            float4 param_94 = packed2;
            float4 param_95 = packed1;
            float4 param_96 = packed2;
            float3 param_97 = local_p;
            float param_98 = CappedCone_get_radiusTop(param_91, param_92);
            float param_99 = CappedCone_get_radiusBottom(param_93, param_94);
            float param_100 = CappedCone_get_height(param_95, param_96);
            d = cappedConeSDF(param_97, param_98, param_99, param_100);
            break;
        }
        case 15:
        {
            float4 param_101 = packed1;
            float4 param_102 = packed2;
            float4 param_103 = packed1;
            float4 param_104 = packed2;
            float3 param_105 = local_p;
            float3 param_106 = Plane_get_normal(param_101, param_102);
            float param_107 = Plane_get_distance(param_103, param_104);
            d = planeSDF(param_105, param_106, param_107);
            break;
        }
        default:
        {
            d = 100.0f;
            break;
        }
    }
    return d;
}

float unionBlend(float d1, float d2)
{
    return min(d1, d2);
}

float intersectBlend(float d1, float d2)
{
    return max(d1, d2);
}

float subtractBlend(float d1, float d2)
{
    return max(-d1, d2);
}

float xorBlend(float d1, float d2)
{
    return max(min(d1, d2), -max(d1, d2));
}

float smoothUnionBlend(float d1, float d2, float k)
{
    float h = clamp(0.5f + ((0.5f * (d2 - d1)) / k), 0.0f, 1.0f);
    return lerp(d2, d1, h) - ((k * h) * (1.0f - h));
}

float smoothIntersectionBlend(float d1, float d2, float k)
{
    float h = clamp(0.5f - ((0.5f * (d2 - d1)) / k), 0.0f, 1.0f);
    return lerp(d2, d1, h) + ((k * h) * (1.0f - h));
}

float smoothSubtractionBlend(float d1, float d2, float k)
{
    float h = clamp(0.5f - ((0.5f * (d2 + d1)) / k), 0.0f, 1.0f);
    return lerp(d2, -d1, h) + ((k * h) * (1.0f - h));
}

hit_info sceneSDF(float3 p)
{
    hit_info hit;
    hit.d = 100.0f;
    SDF_Node _1353;
    _1353.nodeType = _1341_nodes[pc_data_curr_draw_node_idx].nodeType;
    _1353.primType = _1341_nodes[pc_data_curr_draw_node_idx].primType;
    _1353.transform = _1341_nodes[pc_data_curr_draw_node_idx].transform;
    _1353.scale = _1341_nodes[pc_data_curr_draw_node_idx].scale;
    _1353.packed_params[0] = _1341_nodes[pc_data_curr_draw_node_idx].packed_params[0];
    _1353.packed_params[1] = _1341_nodes[pc_data_curr_draw_node_idx].packed_params[1];
    _1353.blend = _1341_nodes[pc_data_curr_draw_node_idx].blend;
    _1353.prim_a = _1341_nodes[pc_data_curr_draw_node_idx].prim_a;
    _1353.prim_b = _1341_nodes[pc_data_curr_draw_node_idx].prim_b;
    _1353.material.diffuse = _1341_nodes[pc_data_curr_draw_node_idx].material.diffuse;
    SDF_Node parent_node = _1353;
    int sp = 0;
    int _1360 = sp;
    sp = _1360 + 1;
    blend_node _1368 = { 0, pc_data_curr_draw_node_idx, parent_node.transform };
    blend_node stack[32];
    stack[_1360] = _1368;
    while (sp > 0)
    {
        int _1379 = sp;
        int _1380 = _1379 - 1;
        sp = _1380;
        blend_node curr_blend_node = stack[_1380];
        if (curr_blend_node.node < 0)
        {
            continue;
        }
        SDF_Node _1394;
        _1394.nodeType = _1341_nodes[curr_blend_node.node].nodeType;
        _1394.primType = _1341_nodes[curr_blend_node.node].primType;
        _1394.transform = _1341_nodes[curr_blend_node.node].transform;
        _1394.scale = _1341_nodes[curr_blend_node.node].scale;
        _1394.packed_params[0] = _1341_nodes[curr_blend_node.node].packed_params[0];
        _1394.packed_params[1] = _1341_nodes[curr_blend_node.node].packed_params[1];
        _1394.blend = _1341_nodes[curr_blend_node.node].blend;
        _1394.prim_a = _1341_nodes[curr_blend_node.node].prim_a;
        _1394.prim_b = _1341_nodes[curr_blend_node.node].prim_b;
        _1394.material.diffuse = _1341_nodes[curr_blend_node.node].material.diffuse;
        SDF_Node node = _1394;
        hit.material = node.material;
        float d = 100.0f;
        if (node.nodeType == 0)
        {
            float SCALE = node.scale;
            float3 param = p;
            float4x4 param_1 = mul(node.transform, curr_blend_node.transform);
            float3 local_p = opTx(param, param_1);
            local_p /= SCALE.xxx;
            float4 packed1 = node.packed_params[0];
            float4 packed2 = node.packed_params[1];
            float3 param_2 = local_p;
            int param_3 = node.primType;
            float4 param_4 = packed1;
            float4 param_5 = packed2;
            d = getPrimitiveSDF(param_2, param_3, param_4, param_5);
            d *= SCALE;
            if (curr_blend_node.blend == 0)
            {
                float param_6 = hit.d;
                float param_7 = d;
                hit.d = unionBlend(param_6, param_7);
            }
            else
            {
                if (curr_blend_node.blend == 1)
                {
                    float param_8 = hit.d;
                    float param_9 = d;
                    hit.d = intersectBlend(param_8, param_9);
                }
                else
                {
                    if (curr_blend_node.blend == 2)
                    {
                        float param_10 = hit.d;
                        float param_11 = d;
                        hit.d = subtractBlend(param_10, param_11);
                    }
                    else
                    {
                        if (curr_blend_node.blend == 3)
                        {
                            float param_12 = hit.d;
                            float param_13 = d;
                            hit.d = xorBlend(param_12, param_13);
                        }
                        else
                        {
                            if (curr_blend_node.blend == 4)
                            {
                                float param_14 = hit.d;
                                float param_15 = d;
                                float param_16 = 0.5f;
                                hit.d = smoothUnionBlend(param_14, param_15, param_16);
                            }
                            else
                            {
                                if (curr_blend_node.blend == 5)
                                {
                                    float param_17 = hit.d;
                                    float param_18 = d;
                                    float param_19 = 0.5f;
                                    hit.d = smoothIntersectionBlend(param_17, param_18, param_19);
                                }
                                else
                                {
                                    float param_20 = hit.d;
                                    float param_21 = d;
                                    float param_22 = 0.5f;
                                    hit.d = smoothSubtractionBlend(param_20, param_21, param_22);
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (sp < 30)
            {
                float4x4 child_transform = curr_blend_node.transform;
                if (node.prim_b >= 0)
                {
                    int _1547 = sp;
                    sp = _1547 + 1;
                    blend_node _1554 = { node.blend, node.prim_b, child_transform };
                    stack[_1547] = _1554;
                }
                if (node.prim_a >= 0)
                {
                    int _1562 = sp;
                    sp = _1562 + 1;
                    blend_node _1569 = { node.blend, node.prim_a, child_transform };
                    stack[_1562] = _1569;
                }
            }
        }
    }
    return hit;
}

hit_info raymarch(Ray ray)
{
    hit_info hit;
    hit.d = 0.0f;
    for (int i = 0; i < 128; i++)
    {
        float3 p = ray.ro + (ray.rd * hit.d);
        float3 param = p;
        hit_info h = sceneSDF(param);
        hit.d += h.d;
        hit.material = h.material;
        bool _1683 = hit.d > 100.0f;
        bool _1690;
        if (!_1683)
        {
            _1690 = hit.d < 0.00999999977648258209228515625f;
        }
        else
        {
            _1690 = _1683;
        }
        if (_1690)
        {
            break;
        }
    }
    return hit;
}

float3 estimateNormal(float3 p)
{
    float3 param = float3(p.x + 0.00999999977648258209228515625f, p.y, p.z);
    float3 param_1 = float3(p.x - 0.00999999977648258209228515625f, p.y, p.z);
    float3 param_2 = float3(p.x, p.y + 0.00999999977648258209228515625f, p.z);
    float3 param_3 = float3(p.x, p.y - 0.00999999977648258209228515625f, p.z);
    float3 param_4 = float3(p.x, p.y, p.z + 0.00999999977648258209228515625f);
    float3 param_5 = float3(p.x, p.y, p.z - 0.00999999977648258209228515625f);
    return normalize(float3(sceneSDF(param).d - sceneSDF(param_1).d, sceneSDF(param_2).d - sceneSDF(param_3).d, sceneSDF(param_4).d - sceneSDF(param_5).d));
}

void comp_main()
{
    float2 uv = float2(gl_GlobalInvocationID.xy) / float2(pc_data_resolution);
    float2 ndcPos = ((uv * 2.0f) - 1.0f.xx) * float2(1.0f, -1.0f);
    float4 nearp = mul(float4(ndcPos, -1.0f, 1.0f), spvInverse(pc_data_view_proj));
    float4 farp = mul(float4(ndcPos, 1.0f, 1.0f), spvInverse(pc_data_view_proj));
    Ray ray;
    ray.ro = nearp.xyz / nearp.w.xxx;
    ray.rd = normalize((farp.xyz / farp.w.xxx) - ray.ro);
    float4 FragColor = float4(1.0f, 0.0f, 1.0f, 0.0f);
    Ray param = ray;
    hit_info hit = raymarch(param);
    if (hit.d < 100.0f)
    {
        float3 lightPos = float3(2.0f, 5.0f, 5.0f);
        float3 p = ray.ro + (ray.rd * hit.d);
        float3 l = normalize(lightPos - p);
        float3 param_1 = p;
        float3 n = normalize(estimateNormal(param_1));
        float3 r = reflect(-l, n);
        float3 v = normalize(ray.ro - p);
        float3 h = normalize(l + v);
        float diffuse = clamp(dot(l, n), 0.0f, 1.0f);
        float spec = pow(max(dot(v, r), 0.0f), 32.0f);
        float3 specular = hit.material.diffuse.xyz * spec;
        float3 diffuseColor = hit.material.diffuse.xyz * diffuse;
        FragColor = float4(diffuseColor + (specular * 10.0f), 1.0f);
        outColorRenderTarget[int2(gl_GlobalInvocationID.xy)] = FragColor;
        return;
    }
}

[numthreads(8, 8, 1)]
void main(SPIRV_Cross_Input stage_input)
{
    gl_GlobalInvocationID = stage_input.gl_GlobalInvocationID;
    comp_main();
}
