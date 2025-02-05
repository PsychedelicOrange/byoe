#version 410 core
// Phong: https://www.shadertoy.com/view/7lj3Rt
// Reference: https://www.shadertoy.com/view/stBcW1
// Added: https://www.shadertoy.com/view/lsc3z4
////////////////////////////////////////////////////////////////////////////////////////
// Constants
#define MAX_STEPS 128
#define RAY_MIN_STEP 0.01
#define RAY_MAX_STEP 100
#define EPSILON 0.01f

////////////////////////////////////////////////////////////////////////////////////////
// Types
struct Ray
{
    vec3 ro;
    vec3 rd;
};

////////////////////////////////////////////////////////////////////////////////////////
// in/out variables

uniform int rocks_idx;
uniform vec4 rocks[100];
uniform ivec2 resolution;

in vec4 nearp;
in vec4 farp;

out vec4 FragColor;
////////////////////////////////////////////////////////////////////////////////////////
// SDF Utils

float sphereSDF(vec3 p, vec4 circle)
{
    return length(p-circle.xyz) - circle.w;
}

float sdBox( vec3 p, vec3 b )
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

////////////////////////////////////////////////////////////////////////////////////////
// SDF Scene description
float sceneSDF(vec3 p)
{
    float hit = RAY_MAX_STEP;
    hit = min(hit, sphereSDF(p, rocks[rocks_idx]));
    return hit;
}

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
// MAIN
void main()
{
	vec2 uv = (gl_FragCoord.xy - (0.5 * resolution.xy))/resolution.y;
    Ray ray;
    ray.ro = nearp.xyz/nearp.w;
    ray.rd = normalize((farp.xyz/farp.w) - ray.ro);

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

        gl_FragDepth = d/100;

        FragColor = vec4(diffuseColor + specular * 10,1.0);  
    }
    else
        gl_FragDepth = 1;

    //FragColor = vec4(uv, 0.0f, 1.0f);
}
