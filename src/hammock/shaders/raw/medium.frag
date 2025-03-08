#version 450

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform Buffer {
    mat4 view;
    mat4 proj;
    vec4 eye;
    vec4 lightPosition;
    float resX;
    float resY;
    float elapsedTime;
};


layout (binding = 1) uniform sampler3D sdfSampler;
layout (binding = 2) uniform sampler3D densityNoiseSampler;
layout (binding = 3) uniform sampler2D curlNoiseSampler;

#define NUM_STEPS 64

#define AABB_MIN vec3(-0.5)
#define AABB_MAX vec3(0.5)

vec3 worldToAABB(vec3 worldPos) {
    return (worldPos - AABB_MIN) / (AABB_MAX - AABB_MIN);
}
vec3 getRayOrigin() {
    return eye.xyz;
}

vec3 getRayDirection() {
    mat4 inverseView = inverse(view);

    // Calculate aspect ratio
    float aspectRatio = resX / resY;

    vec2 uv = gl_FragCoord.xy / vec2(resX, resY);
    uv -= 0.5;
    uv.x *= aspectRatio;
    uv.y *= -1.0;

    // Ray Direction
    vec3 rayDir = normalize(vec3(uv, -1.0));

    // Transform ray direction by the camera's orientation
    rayDir = (inverseView * vec4(rayDir, 0.0)).xyz;

    return rayDir;
}

vec2 intersectRayAABB(vec3 rayOrigin, vec3 rayDir, vec3 aabbMin, vec3 aabbMax) {
    vec3 tMin = (aabbMin - rayOrigin) / rayDir;
    vec3 tMax = (aabbMax - rayOrigin) / rayDir;

    // Ensure tMin is always the entry point and tMax is the exit point
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);

    // Find the largest t1 (entry) and smallest t2 (exit)
    float t_enter = max(t1.x, max(t1.y, t1.z));
    float t_exit  = min(t2.x, min(t2.y, t2.z));

    // No intersection if t_exit is behind the ray or if entry is after exit
    if (t_exit < 0.0 || t_enter > t_exit) {
        return vec2(-1.0, -1.0); // No hit
    }

    return vec2(t_enter, t_exit); // Distance to AABB entry and exit
}

bool isPointInsideAABB(vec3 point, vec3 minPoint, vec3 maxPoint) {
    return all(greaterThanEqual(point, minPoint)) && all(lessThanEqual(point, maxPoint));
}

float remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax)
{
    return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float sdf(vec3 p) {
    vec3 uvw = worldToAABB(p); // Map world pos to AABB UVW
    uvw = clamp(uvw, vec3(0.001), vec3(0.999)); // Prevent out-of-bounds

    return texture(sdfSampler, uvw).r; // Sample SDF
}

float sampleDensity(vec3 p) {
    float dist = sdf(p);
    if (dist > 0.0) return 0.0;


    vec4 noise = texture(densityNoiseSampler, p);
    return noise.r;
}

void main() {
    vec3 rayOrigin = getRayOrigin();
    vec3 rayDirection = getRayDirection();

    vec2 intersection = intersectRayAABB(rayOrigin, rayDirection, AABB_MIN, AABB_MAX);
    if (intersection.y < 0.0) {
        outColor = vec4(0.0); // No intersection
        return;
    }

    float stepSize = (intersection.y - intersection.x) / float(NUM_STEPS);
    vec3 p = rayOrigin + rayDirection * intersection.x; // Start inside AABB

    float transmittance = 1.0;

    for (int i = 0; i < NUM_STEPS; i++) {
        vec3 uvw = worldToAABB(p); // Convert to AABB-space
        float sampledDensity = sampleDensity(p);

        if (sampledDensity > 0.0) {
            float t = exp(-sampledDensity * stepSize);
            transmittance *= t;
        }

        if (transmittance < 0.01) break;

        p += rayDirection * stepSize; // Step inside the AABB
    }

    outColor = vec4(1.0 - transmittance);
}