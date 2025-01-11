#version 410 core
// https://iquilezles.org/articles/distfunctions/
////////////////////////////////////////////////////////////////////////////////////////
// Constants
#define MAX_STEPS 128
#define RAY_MIN_STEP 0.01
#define RAY_MAX_STEP 100.0
#define EPSILON 0.01

#define MAX_STACK_SIZE 32

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
    vec4 pos;
    vec4 scale;

    int blend;       
    int prim_a;     // Index of the left child node
    int prim_b;    // Index of the right child node

    int is_ref_node;
    SDF_Material material;
};

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
// SDF Primitive Functions
float sphereSDF(vec3 p, vec3 pos, float scale) {
    return length(p - pos) - scale;
}

float boxSDF(vec3 p, vec3 pos, vec3 size) {
    vec3 q = abs(p - pos) - size;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

// TODO: Define other SDF functions here
////////////////////////////////////////////////////////////////////////////////////////
// SDF Combination Operations
float unionOp(float d1, float d2) {
    return min(d1, d2);
}

float intersectOp(float d1, float d2) {
    return max(d1, d2);
}

float subtractOp(float d1, float d2) {
    return max(d1, -d2);
}

float opSmoothUnion( float d1, float d2, float k )
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

float opSmoothSubtraction( float d1, float d2, float k )
{
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h);
}

float opSmoothIntersection( float d1, float d2, float k )
{
    float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) + k*h*(1.0-h);
}

// TODO: Define other operation funtions here
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

    // TEST CODE!!! TEST CODE!!! TEST CODE!!!
    // hit.material.diffuse = vec4(1, 0, 0, 1);
    // float d = sphereSDF(p, vec3(1, 1, 0), 1.5f);
    // hit.d = unionOp(hit.d, d);
    // d = sphereSDF(p, vec3(0, 0, 0), 1.5f);
    // hit.d = intersectOp(hit.d, d);
    // return hit;
    // TEST CODE!!! TEST CODE!!! TEST CODE!!!

    // Explicit stack to emulate tree traversal
    blend_node stack[MAX_STACK_SIZE];
    int sp = 0; // Stack pointer
    stack[sp++] = blend_node(0, curr_draw_node_idx);

    while (sp > 0) {
        blend_node node_index = stack[--sp]; // Pop node index
        if (node_index.node < 0) continue;

        SDF_Node node = nodes[node_index.node];
        float d = RAY_MAX_STEP;
        // Evaluate the current node
        if(node.nodeType == 0) {
            if (node.primType == 0) { // Sphere
                d = sphereSDF(p, node.pos.xyz, node.scale.x);
            } else if (node.primType == 1) { // Box
                d = boxSDF(p, node.pos.xyz, node.scale.xyz);
            } else {
                d = RAY_MAX_STEP; // Default for unknown types
            }

            // Apply the blend b/w primitives
            if (node_index.blend == 0) {
                hit.d = unionOp(hit.d, d);
            } else if (node_index.blend == 1) {
                hit.d = opSmoothUnion(hit.d, d, 0.5f);
            } else if (node_index.blend == 2) {
                hit.d = subtractOp(hit.d, d);
            } else if (node_index.blend == 3) {
                hit.d = subtractOp(hit.d, d);
            } else {
                hit.d = RAY_MAX_STEP;
            }
        }
        // it it's an Object process to blend and do stuff etc.
        else {
            // Push child nodes onto the stack
            if (sp < MAX_STACK_SIZE - 2) {
                if (node.prim_b >= 0) stack[sp++] = blend_node(node.blend, node.prim_b);
                if (node.prim_a >= 0) stack[sp++] = blend_node(node.blend, node.prim_a);
            }
        }

        hit.material = node.material;
    }

    return hit;
}
////////////////////////////////////////////////////////////////////////////////////////
// Rendering related functions
// Normal Estimation
vec3 estimateNormal(vec3 p) {
    return normalize(vec3(
        sceneSDF(vec3(p.x + EPSILON, p.y, p.z)).d  - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)).d,
        sceneSDF(vec3(p.x, p.y + EPSILON, p.z)).d  - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)).d,
        sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)).d - sceneSDF(vec3(p.x, p.y, p.z - EPSILON)).d
    ));
}
////////////////////////////////////////////////////////////////////////////////////////
// Ray Marching

hit_info raymarch(Ray ray){
    hit_info hit;
    hit.d = 0;
    for(int i = 0; i < MAX_STEPS; i++)
    {
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
        vec3 lightPos = vec3(0, 5, 5);

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
    }
    else
        gl_FragDepth = 1;

}