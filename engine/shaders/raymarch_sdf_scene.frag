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

    // Explicit stack to emulate tree traversal
    int stack[MAX_STACK_SIZE];
    int sp = 0; // Stack pointer
    stack[sp++] = 0; // Push root node (assumes root is at index 0)

    while (sp > 0) {
        int node_index = stack[--sp]; // Pop node index
        if (node_index < 0) continue;

        SDF_Node node = nodes[node_index];

        // Evaluate the current node
        float d;
        if (node.type == 0) { // Sphere
            d = sphereSDF(p, node.params);
        } else if (node.nodeType == 1) { // Box
            d = boxSDF(p, node.params.xyz);
        } else {
            d = RAY_MAX_STEP; // Default for unknown types
        }

        break;

        // Apply the operation
        if (node.op == 0) { // Union
            result = unionOp(result, d);
        } else if (node.op == 1) { // Intersection
            result = intersectOp(result, d);
        } else if (node.op == 2) { // Subtraction
            result = subtractOp(result, d);
        } else { // No operation
            result = d;
        }

        // Push child nodes onto the stack
        if (sp < MAX_STACK_SIZE - 2) {
            if (node.left >= 0) stack[sp++] = node.left;
            if (node.right >= 0) stack[sp++] = node.right;
        }
    }

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
float raymarch(Ray ray) {
    float depth = 0.0;

    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 p = ray.ro + ray.rd * depth;
        float dist = sceneSDF(p);

        if (dist < RAY_MIN_STEP) {
            return depth; // Hit the surface
        }

        depth += dist;

        if (depth > RAY_MAX_STEP) {
            break; // Exit if the ray exceeds max depth
        }
    }

    return RAY_MAX_STEP; // Did not hit anything
}

////////////////////////////////////////////////////////////////////////////////////////
// Main
void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;

    Ray ray;
    ray.ro = nearp.xyz / nearp.w; // Ray origin in world space
    ray.rd = normalize((farp.xyz / farp.w) - ray.ro); // Ray direction

    FragColor = vec4(uv, 0.0f, 1.0f);
    return;

    // Perform ray marching
    float depth = raymarch(ray);

    if (depth < RAY_MAX_STEP) {
        vec3 p = ray.ro + ray.rd * depth;

        vec3 n = estimateNormal(p);
        vec3 l = normalize(dir_light_pos - p);
        vec3 v = normalize(ray.ro - p);
        vec3 r = reflect(-l, n);

        // Lighting model: Basic Phong Lighting (diffuse and specular only for directional light)

        float diff = max(dot(n, l), 0.0); // Diffuse shading
        float spec = pow(max(dot(r, v), 0.0), 32.0); // Specular shading

        // using a default green material for testing
        vec3 color = vec3(0.3, 0.5, 0.8) * diff + vec3(1.0) * spec;

        FragColor = vec4(color, 1.0);

        // Write to depth texture (scale steps taken by range)
        gl_FragDepth = depth / RAY_MAX_STEP;
    } else {
        // Miss? use background color
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        // Write to depth texture no hit = no depth 
        gl_FragDepth = 1.0;
    }
}