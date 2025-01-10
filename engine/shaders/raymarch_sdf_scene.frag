#version 410 core

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

struct SDF_Node {
    int nodeType; 
    
    int primType;
    vec4 params;  // pos + radius

    int op;       
    int left;     // Index of the left child node
    int right;    // Index of the right child node
};

////////////////////////////////////////////////////////////////////////////////////////
// Uniforms
uniform ivec2 resolution;              
uniform int sdf_node_count;       
uniform vec3 dir_light_pos;  
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
float sphereSDF(vec3 p, vec4 params) {
    return length(p - params.xyz) - params.w;
}

float boxSDF(vec3 p, vec3 size) {
    vec3 q = abs(p) - size;
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
// TODO: Define other operation funtions here
////////////////////////////////////////////////////////////////////////////////////////
// Iterative Scene SDF Evaluation
float sceneSDF(vec3 p) {
    float result = RAY_MAX_STEP;

    result = sphereSDF(p, vec4(0, 0, 0, 1));
    // hit = hit + sin(2*p.x)*sin(2*p.y)*sin(2*p.z);
    // hit = min(hit, sdCapsule(p, vec3(0.1), vec3(0.8), 0.5f));
    //return result;

    // Explicit stack to emulate tree traversal
    int stack[MAX_STACK_SIZE];
    int sp = 0; // Stack pointer
    stack[sp++] = 0; // Push root node (assumes root is at index 0)

    // while (sp > 0) {
        // int node_index = stack[--sp]; // Pop node index
        // if (node_index < 0) continue;

        SDF_Node node = nodes[0];

        // Evaluate the current node
        float d;
        if (node.primType == 1) { // Sphere
            d = sphereSDF(p, node.params);
        } else if (node.primType == 0) { // Box
            d = boxSDF(p, node.params.xyz);
        } else {
            d = RAY_MAX_STEP; // Default for unknown types
        }
        d = sphereSDF(p, node.params);

        result = d;
        // break;

        // // Apply the operation
        // if (node.op == 0) { // Union
        //     result = unionOp(result, d);
        // } else if (node.op == 1) { // Intersection
        //     result = intersectOp(result, d);
        // } else if (node.op == 2) { // Subtraction
        //     result = subtractOp(result, d);
        // } else { // No operation
        //     result = d;
        // }

        // // Push child nodes onto the stack
        // if (sp < MAX_STACK_SIZE - 2) {
        //     if (node.left >= 0) stack[sp++] = node.left;
        //     if (node.right >= 0) stack[sp++] = node.right;
        // }
    // }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////
// Rendering related functions
// Normal Estimation
vec3 estimateNormal(vec3 p) {
    return normalize(vec3(
        sceneSDF(vec3(p.x + EPSILON, p.y, p.z))  - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)),
        sceneSDF(vec3(p.x, p.y + EPSILON, p.z))  - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)),
        sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)) - sceneSDF(vec3(p.x, p.y, p.z - EPSILON))
    ));
}
////////////////////////////////////////////////////////////////////////////////////////
// Ray Marching

float raymarch(Ray ray){
    float depth = 0;
    for(int i = 0; i < MAX_STEPS; i++)
    {
        vec3 p = ray.ro + ray.rd * depth;
        float hit = sceneSDF(p);
        depth += hit;
        if(depth > RAY_MAX_STEP || depth < RAY_MIN_STEP) break;
    }
    return depth;
}

////////////////////////////////////////////////////////////////////////////////////////
// Main
void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;

    Ray ray;
    ray.ro = nearp.xyz / nearp.w; // Ray origin in world space
    ray.rd = normalize((farp.xyz / farp.w) - ray.ro); // Ray direction

    // Perform ray marching
    float d  = raymarch(ray);
    
    if(d < RAY_MAX_STEP)
    {
        vec3 lightPos = vec3(0, 5, 5);

        vec3 p = ray.ro + ray.rd * d;

        vec3 l = normalize(lightPos - p); // light vector
        vec3 n = normalize(estimateNormal(p));
        vec3 r = reflect(-l, n);
        vec3 v = normalize(ray.ro - p);

        vec3 h = normalize(l + v); // the `half-angle` vector

        float diffuse  = clamp(dot(l, n), 0., 1.);
        float spec = pow(max(dot(v, r), 0.0), 32);

		vec3 specular = vec3(0.3, 0.5, 0.2) * spec;
		vec3 diffuseColor = diffuse * vec3(0.3, 0.5, 0.2);

        gl_FragDepth = d / RAY_MAX_STEP;

        FragColor = vec4(diffuseColor + specular * 10,1.0);  
    }
    else
        gl_FragDepth = 1;

}