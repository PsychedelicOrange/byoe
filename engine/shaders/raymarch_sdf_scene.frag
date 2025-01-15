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

// Blend modes
#define SDF_BLEND_UNION 0
#define SDF_BLEND_SMOOTH_UNION 1
#define SDF_BLEND_INTERSECTION 2
#define SDF_BLEND_SUBTRACTION 3

// Operations

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
float sphereSDF(vec3 p, float radius) {
    return length(p) - radius;
}

float boxSDF(vec3 p, vec3 size) {
    vec3 q = abs(p) - size;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float roundBoxSDF( vec3 p, vec3 b, float r )
{
  vec3 q = abs(p) - b + r;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

float boxFrameSDF( vec3 p, vec3 b, float e )
{
       p = abs(p  )-b;
  vec3 q = abs(p+e)-e;
  return min(min(
      length(max(vec3(p.x,q.y,q.z),0.0))+min(max(p.x,max(q.y,q.z)),0.0),
      length(max(vec3(q.x,p.y,q.z),0.0))+min(max(q.x,max(p.y,q.z)),0.0)),
      length(max(vec3(q.x,q.y,p.z),0.0))+min(max(q.x,max(q.y,p.z)),0.0));
}

float torusSDF( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

float cappedTorusSDF( vec3 p, vec2 sc, float ra, float rb)
{
  p.x = abs(p.x);
  float k = (sc.y*p.x>sc.x*p.y) ? dot(p.xy,sc) : length(p.xy);
  return sqrt( dot(p,p) + ra*ra - 2.0*ra*k ) - rb;
}

float linkSDF( vec3 p, float le, float r1, float r2 )
{
  vec3 q = vec3( p.x, max(abs(p.y)-le,0.0), p.z );
  return length(vec2(length(q.xy)-r1,q.z)) - r2;
}

float infCylinderSDF( vec3 p, vec3 c )
{
  return length(p.xz-c.xy)-c.z;
}

float coneSDF( vec3 p, vec2 c, float h )
{
  float q = length(p.xz);
  return max(dot(c.xy,vec2(q,p.y)),-h-p.y);
}

float planeSDF( vec3 p, vec3 n, float h )
{
  // n must be normalized
  return dot(p,n) + h;
}

float hexPrismSDF( vec3 p, vec2 h )
{
  const vec3 k = vec3(-0.8660254, 0.5, 0.57735);
  p = abs(p);
  p.xy -= 2.0*min(dot(k.xy, p.xy), 0.0)*k.xy;
  vec2 d = vec2(
       length(p.xy-vec2(clamp(p.x,-k.z*h.x,k.z*h.x), h.x))*sign(p.y-h.x),
       p.z-h.y );
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float triPrismSDF( vec3 p, vec2 h )
{
  vec3 q = abs(p);
  return max(q.z-h.y,max(q.x*0.866025+p.y*0.5,-p.y)-h.x*0.5);
}

float capsuleSDF( vec3 p, vec3 a, vec3 b, float r )
{
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

float verticalCapsuleSDF( vec3 p, float h, float r )
{
  p.y -= clamp( p.y, 0.0, h );
  return length( p ) - r;
}

float cappedCylinderSDF( vec3 p, float h, float r )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(r,h);
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float roundedCylinderSDF( vec3 p, float ra, float rb, float h )
{
  vec2 d = vec2( length(p.xz)-2.0*ra+rb, abs(p.y) - h );
  return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rb;
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
    return max(-d1, d2);
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

    // Explicit stack to emulate tree traversal
    blend_node stack[MAX_GPU_STACK_SIZE];
    int sp = 0; // Stack pointer
    stack[sp++] = blend_node(SDF_BLEND_UNION, curr_draw_node_idx);

    while (sp > 0) {
        blend_node curr_blend_node = stack[--sp]; // Pop node index
        if (curr_blend_node.node < 0) continue;

        SDF_Node node = nodes[curr_blend_node.node];

        float d = RAY_MAX_STEP;
        // Evaluate the current node
        if(node.nodeType == 0) {
            if (node.primType == 0) { // Sphere
                d = sphereSDF(p, node.scale.x);
            } else if (node.primType == 1) { // Box
                d = boxSDF(p, node.scale.xyz);
            }

            // Apply the blend b/w primitives
            if (curr_blend_node.blend == SDF_BLEND_UNION) {
                hit.d = unionOp(hit.d, d);
            } else if (curr_blend_node.blend == SDF_BLEND_SMOOTH_UNION) {
                hit.d = opSmoothUnion(hit.d, d, 0.5f);
            } else if (curr_blend_node.blend == SDF_BLEND_INTERSECTION) {
                hit.d = intersectOp(hit.d, d);
            } else if (curr_blend_node.blend == SDF_BLEND_SUBTRACTION) {
                hit.d = subtractOp(hit.d, d);
            }
        }
        else {            
            if (sp < MAX_GPU_STACK_SIZE - 2) {
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
    else
        gl_FragDepth = 1;

}