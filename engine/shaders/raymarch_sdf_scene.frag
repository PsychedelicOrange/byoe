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

float cappedConeSDF( vec3 p, float h, float r1, float r2 )
{
  vec2 q = vec2( length(p.xz), p.y );
  vec2 k1 = vec2(r2,h);
  vec2 k2 = vec2(r2-r1,2.0*h);
  vec2 ca = vec2(q.x-min(q.x,(q.y<0.0)?r1:r2), abs(q.y)-h);
  vec2 cb = q - k1 + k2*clamp( dot(k1-q,k2)/dot2(k2), 0.0, 1.0 );
  float s = (cb.x<0.0 && ca.y<0.0) ? -1.0 : 1.0;
  return s*sqrt( min(dot2(ca),dot2(cb)) );
}

float cutSphereSDF( vec3 p, float r, float h )
{
  // sampling independent computations (only depend on shape)
  float w = sqrt(r*r-h*h);

  // sampling dependant computations
  vec2 q = vec2( length(p.xz), p.y );
  float s = max( (h-r)*q.x*q.x+w*w*(h+r-2.0*q.y), h*q.x-w*q.y );
  return (s<0.0) ? length(q)-r :
         (q.x<w) ? h - q.y     :
                   length(q-vec2(w,h));
}

float cutHollowSphereSDF( vec3 p, float r, float h, float t )
{
  // sampling independent computations (only depend on shape)
  float w = sqrt(r*r-h*h);
  
  // sampling dependant computations
  vec2 q = vec2( length(p.xz), p.y );
  return ((h*q.x<w*q.y) ? length(q-vec2(w,h)) : 
                          abs(length(q)-r) ) - t;
}

float deathStarSDF( vec3 p2, float ra, float rb, float d )
{
  // sampling independent computations (only depend on shape)
  float a = (ra*ra - rb*rb + d*d)/(2.0*d);
  float b = sqrt(max(ra*ra-a*a,0.0));
	
  // sampling dependant computations
  vec2 p = vec2( p2.x, length(p2.yz) );
  if( p.x*b-p.y*a > d*max(b-p.y,0.0) )
    return length(p-vec2(a,b));
  else
    return max( (length(p            )-ra),
               -(length(p-vec2(d,0.0))-rb));
}

float roundConeSDF( vec3 p, float r1, float r2, float h )
{
  // sampling independent computations (only depend on shape)
  float b = (r1-r2)/h;
  float a = sqrt(1.0-b*b);

  // sampling dependant computations
  vec2 q = vec2( length(p.xz), p.y );
  float k = dot(q,vec2(-b,a));
  if( k<0.0 ) return length(q) - r1;
  if( k>a*h ) return length(q-vec2(0.0,h)) - r2;
  return dot(q, vec2(a,b) ) - r1;
}

float octahedronSDF( vec3 p, float s )
{
  p = abs(p);
  float m = p.x+p.y+p.z-s;
  vec3 q;
       if( 3.0*p.x < m ) q = p.xyz;
  else if( 3.0*p.y < m ) q = p.yzx;
  else if( 3.0*p.z < m ) q = p.zxy;
  else return m*0.57735027;
    
  float k = clamp(0.5*(q.z-q.y+s),0.0,s); 
  return length(vec3(q.x,q.y-s+k,q.z-k)); 
}

float pyramidSDF( vec3 p, float h )
{
  float m2 = h*h + 0.25;
    
  p.xz = abs(p.xz);
  p.xz = (p.z>p.x) ? p.zx : p.xz;
  p.xz -= 0.5;

  vec3 q = vec3( p.z, h*p.y - 0.5*p.x, h*p.x + 0.5*p.y);
   
  float s = max(-q.x,0.0);
  float t = clamp( (q.y-0.5*p.z)/(m2+0.25), 0.0, 1.0 );
    
  float a = m2*(q.x+s)*(q.x+s) + q.y*q.y;
  float b = m2*(q.x+0.5*t)*(q.x+0.5*t) + (q.y-m2*t)*(q.y-m2*t);
    
  float d2 = min(q.y,-q.x*m2-q.y*0.5) > 0.0 ? 0.0 : min(a,b);
    
  return sqrt( (d2+q.z*q.z)/m2 ) * sign(max(q.z,-p.y));
}

float triangleSDF( vec3 p, vec3 a, vec3 b, vec3 c )
{
  vec3 ba = b - a; vec3 pa = p - a;
  vec3 cb = c - b; vec3 pb = p - b;
  vec3 ac = a - c; vec3 pc = p - c;
  vec3 nor = cross( ba, ac );

  return sqrt(
    (sign(dot(cross(ba,nor),pa)) +
     sign(dot(cross(cb,nor),pb)) +
     sign(dot(cross(ac,nor),pc))<2.0)
     ?
     min( min(
     dot2(ba*clamp(dot(ba,pa)/dot2(ba),0.0,1.0)-pa),
     dot2(cb*clamp(dot(cb,pb)/dot2(cb),0.0,1.0)-pb) ),
     dot2(ac*clamp(dot(ac,pc)/dot2(ac),0.0,1.0)-pc) )
     :
     dot(nor,pa)*dot(nor,pa)/dot2(nor) );
}

float quadSDF( vec3 p, vec3 a, vec3 b, vec3 c, vec3 d )
{
  vec3 ba = b - a; vec3 pa = p - a;
  vec3 cb = c - b; vec3 pb = p - b;
  vec3 dc = d - c; vec3 pc = p - c;
  vec3 ad = a - d; vec3 pd = p - d;
  vec3 nor = cross( ba, ad );

  return sqrt(
    (sign(dot(cross(ba,nor),pa)) +
     sign(dot(cross(cb,nor),pb)) +
     sign(dot(cross(dc,nor),pc)) +
     sign(dot(cross(ad,nor),pd))<3.0)
     ?
     min( min( min(
     dot2(ba*clamp(dot(ba,pa)/dot2(ba),0.0,1.0)-pa),
     dot2(cb*clamp(dot(cb,pb)/dot2(cb),0.0,1.0)-pb) ),
     dot2(dc*clamp(dot(dc,pc)/dot2(dc),0.0,1.0)-pc) ),
     dot2(ad*clamp(dot(ad,pd)/dot2(ad),0.0,1.0)-pd) )
     :
     dot(nor,pa)*dot(nor,pa)/dot2(nor) );
}

// TODO: Define other SDF functions here
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
        hit.material = node.material;

        float d = RAY_MAX_STEP;
        // Evaluate the current node
        if(node.nodeType == 0) {
            if (node.primType == 0) { // Sphere
                d = sphereSDF(p - node.pos.xyz, node.scale.x);
            } else if (node.primType == 1) { // Box
                d = boxSDF(p - node.pos.xyz, node.scale.xyz);
            }

            // Apply the blend b/w primitives
            if (curr_blend_node.blend == SDF_BLEND_UNION) {
                hit.d = unionBlend(hit.d, d);
            } else if (curr_blend_node.blend == SDF_BLEND_SMOOTH_UNION) {
                hit.d = smoothUnionBlend(hit.d, d, 0.5f);
            } else if (curr_blend_node.blend == SDF_BLEND_INTERSECTION) {
                hit.d = intersectBlend(hit.d, d);
            } else if (curr_blend_node.blend == SDF_BLEND_SUBTRACTION) {
                hit.d = subtractBlend(hit.d, d);
            }
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