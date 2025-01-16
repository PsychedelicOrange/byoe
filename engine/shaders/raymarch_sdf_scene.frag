#version 410 core
// https://iquilezles.org/articles/distfunctions/
////////////////////////////////////////////////////////////////////////////////////////
// Constants
// Ray Marching settings
#define MAX_STEPS 128
#define RAY_MIN_STEP 0.01
#define RAY_MAX_STEP 100.0
#define EPSILON 0.01

#define MAX_GPU_STACK_SIZE 32

#define MAX_PACKED_PARAM_VECS 2

/// Primitives
#define SDF_PRIM_Sphere          0
#define SDF_PRIM_Box             1
#define SDF_PRIM_RoundedBox      2
#define SDF_PRIM_BoxFrame        3
#define SDF_PRIM_Torus           4
#define SDF_PRIM_TorusCapped     5
#define SDF_PRIM_Capsule         6
#define SDF_PRIM_Cylinder        7
#define SDF_PRIM_Ellipsoid       8
#define SDF_PRIM_Quad            9
#define SDF_PRIM_HexagonalPrism  10
#define SDF_PRIM_TriangularPrism 11
#define SDF_PRIM_Cone            12
#define SDF_PRIM_ConeSection     13
#define SDF_PRIM_Plane           14
#define SDF_PRIM_RoundedCylinder 15
#define SDF_PRIM_SolidAngle      16
#define SDF_PRIM_Line            17
#define SDF_PRIM_RoundedCone     18
#define SDF_PRIM_VerticalCapsule 19
#define SDF_PRIM_CappedCone      20
#define SDF_PRIM_CappedTorus     21
#define SDF_PRIM_CappedCylinder  22
#define SDF_PRIM_CappedPlan      23

// Node Type
#define SDF_NODE_PRIMITIVE 0
#define SDF_NODE_OBJECT    1

// Blend modes
#define SDF_BLEND_UNION                 0
#define SDF_BLEND_INTERSECTION          1
#define SDF_BLEND_SUBTRACTION           2
#define SDF_BLEND_XOR                   3
#define SDF_BLEND_SMOOTH_UNION          4
#define SDF_BLEND_SMOOTH_INTERSECTION   5
#define SDF_BLEND_SMOOTH_SUBTRACTION    6

// Operation Modes
#define SDF_OP_DISTORTION 0
#define SDF_OP_ELONGATION 1

////////////////////////////////////////////////////////////////////////////////////////
// Types
struct Ray {
    vec3 ro; 
    vec3 rd; 
};

struct SDF_Material {
    vec4 diffuse; // rgba
};

struct SDF_Node {
    int nodeType;
    
    int primType;
    mat4 transform; // Pos + Rotation only
    float scale;    // Uniform Scaling

    vec4 packed_params[2];

    int blend;
    int prim_a;
    int prim_b;

    SDF_Material material;
};
////////////////////////////////////////////////////////////////////////////////////////
// Params unpacking util functions
// signature: return_type (PRIM_TYPE_NAME)_get_param(XXX)();
//-----------------------------
// Sphere
float Sphere_get_radius(vec4 packed1, vec4 packed2)
{
    return packed1.x;
}
//-----------------------------
// Box
vec3 Box_get_dimensions(vec4 packed1, vec4 packed2)
{
    return packed1.xyz;
}

////////////////////////////////////////////////////////////////////////////////////////
// Uniforms
uniform ivec2 resolution;              
uniform int sdf_node_count;       
uniform vec3 dir_light_pos;  
uniform int curr_draw_node_idx;
layout(std140) uniform SDFScene {
    SDF_Node nodes[100];
};        
////////////////////////////////////////////////////////////////////////////////////////
// Vertex Input vars
in vec4 nearp;
in vec4 farp;
////////////////////////////////////////////////////////////////////////////////////////
// Out Render Targets
out vec4 FragColor;
////////////////////////////////////////////////////////////////////////////////////////
// Helper 
float dot2( in vec2 v ) { return dot(v,v); }
float dot2( in vec3 v ) { return dot(v,v); }
float ndot( in vec2 a, in vec2 b ) { return a.x*b.x - a.y*b.y; }


// SDF Primitive Functions
float sphereSDF(vec3 p, float radius) {
    return length(p) - radius;
}

float boxSDF(vec3 p, vec3 size) {
    vec3 q = abs(p) - size;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

////////////////////////////////////////////////////////////////////////////////////////
// SDF Combination Operations
float unionBlend(float d1, float d2) {
    return min(d1, d2);
}

float intersectBlend(float d1, float d2) {
    return max(d1, d2);
}

float subtractBlend(float d1, float d2) {
    return max(-d1, d2);
}

float xorBlend(float d1, float d2 )
{
    return max(min(d1,d2),-max(d1,d2));
}

float smoothUnionBlend( float d1, float d2, float k )
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

float smoothSubtractionBlend( float d1, float d2, float k )
{
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h);
}

float smoothIntersectionBlend( float d1, float d2, float k )
{
    float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) + k*h*(1.0-h);
}

////////////////////////////////////////////////////////////////////////////////////////
// TODO: Define other operation funtions here

////////////////////////////////////////////////////////////////////////////////////////
// Positioning
// Translate, Rotate and Uniform Scaling
vec3 opTx(vec3 p, mat4 t)
{
    return vec3(inverse(t) * vec4(p, 1.0f));
}

////////////////////////////////////////////////////////////////////////////////////////
// Iterative Scene SDF Evaluation
struct hit_info
{
    float d;
    SDF_Material material;
};

struct blend_node {
    int blend;
    int node;
};

hit_info sceneSDF(vec3 p) {
    hit_info hit;
    hit.d = RAY_MAX_STEP;

    // Explicit stack to emulate tree traversal
    blend_node stack[MAX_GPU_STACK_SIZE];
    int sp = 0; // Stack pointer
    stack[sp++] = blend_node(SDF_BLEND_UNION, curr_draw_node_idx);

    SDF_Node parent_node = nodes[curr_draw_node_idx];

    while (sp > 0) {
        blend_node curr_blend_node = stack[--sp]; // Pop node index
        if (curr_blend_node.node < 0) continue;

        SDF_Node node = nodes[curr_blend_node.node];
        hit.material = node.material;

        float d = RAY_MAX_STEP;
        // Evaluate the current node
        if(node.nodeType == SDF_NODE_PRIMITIVE) {
            float SCALE = 1.0f;
            // Translate/Rotate
            p = opTx(p, node.transform);
            //p /= SCALE;

            if (node.primType == SDF_PRIM_Sphere) { 
                d = sphereSDF(p, Sphere_get_radius(node.packed_params[0], node.packed_params[1]));
            } else if (node.primType == SDF_PRIM_Box) { 
                d = boxSDF(p, Box_get_dimensions(node.packed_params[0], node.packed_params[1]));
            }

            // Scaling 
            //d *= SCALE;

            // Apply the blend b/w primitives
            if (curr_blend_node.blend == SDF_BLEND_UNION)
                hit.d = unionBlend(hit.d, d);
            else if (curr_blend_node.blend == SDF_BLEND_INTERSECTION)
                hit.d = intersectBlend(hit.d, d);
            else if (curr_blend_node.blend == SDF_BLEND_SUBTRACTION)
                hit.d = subtractBlend(hit.d, d);
            else if (curr_blend_node.blend == SDF_BLEND_XOR)
                hit.d = xorBlend(hit.d, d);
            else if (curr_blend_node.blend == SDF_BLEND_SMOOTH_UNION)
                hit.d = smoothUnionBlend(hit.d, d, 0.5f);
            else if (curr_blend_node.blend == SDF_BLEND_SMOOTH_INTERSECTION)
                hit.d = smoothIntersectionBlend(hit.d, d, 0.5f);
            else
                hit.d = smoothSubtractionBlend(hit.d, d, 0.5f);
            // TODO: Apply per node transformations here on the cummulative SDF result
        }
        else {            
            if (sp < MAX_GPU_STACK_SIZE - 2) {
                if (node.prim_b >= 0) stack[sp++] = blend_node(node.blend, node.prim_b);
                if (node.prim_a >= 0) stack[sp++] = blend_node(node.blend, node.prim_a);
            }
        }
    }
    return hit;
}
////////////////////////////////////////////////////////////////////////////////////////
// Rendering related functions
// Normal Estimation N = (n + delta) - (n - delta)
vec3 estimateNormal(vec3 p) {
    return normalize(vec3(
        sceneSDF(vec3(p.x + EPSILON, p.y, p.z)).d  - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)).d,
        sceneSDF(vec3(p.x, p.y + EPSILON, p.z)).d  - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)).d,
        sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)).d - sceneSDF(vec3(p.x, p.y, p.z - EPSILON)).d
    ));
}
////////////////////////////////////////////////////////////////////////////////////////
// Ray Marching
hit_info raymarch(Ray ray) {
    hit_info hit;
    hit.d = 0;
    for(int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ray.ro + ray.rd * hit.d;
        hit_info h = sceneSDF(p);
        hit.d += h.d;
        hit.material = h.material;
        if(hit.d > RAY_MAX_STEP || hit.d < RAY_MIN_STEP) break;
    }
    return hit;
}

////////////////////////////////////////////////////////////////////////////////////////
// Main
void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;

    Ray ray;
    ray.ro = nearp.xyz / nearp.w; // Ray origin in world space
    ray.rd = normalize((farp.xyz / farp.w) - ray.ro); // Ray direction

    hit_info hit  = raymarch(ray);
    
    if(hit.d < RAY_MAX_STEP)
    {
        vec3 lightPos = vec3(2, 5, 5);

        vec3 p = ray.ro + ray.rd * hit.d;

        vec3 l = normalize(lightPos - p);
        vec3 n = normalize(estimateNormal(p));
        vec3 r = reflect(-l, n);
        vec3 v = normalize(ray.ro - p);

        vec3 h = normalize(l + v); 

        float diffuse  = clamp(dot(l, n), 0., 1.);
        float spec = pow(max(dot(v, r), 0.0), 32);

        vec3 specular = hit.material.diffuse.xyz * spec;
        vec3 diffuseColor = diffuse * hit.material.diffuse.xyz;

        gl_FragDepth = hit.d / RAY_MAX_STEP;

        FragColor = vec4(diffuseColor + specular * 10, 1.0f);  

        // FragColor = vec4(hit.d, hit.d, hit.d, 1.0f); 
    }
    else {
        gl_FragDepth = 1;
    }
}