static const float PI = 3.14159265358979323846;

// Types

struct SunAndSkyBuffer{
    float4 lightColor;
    float4 lightDirection;
    float4 skyColorZenith;
    float4 skyColorHorizon;
    float4 windDirection;
};

struct CameraBuffer {
    float4x4 inv_view;
    float4x4 inv_proj;
    float4x4 invViewProj;
    float4 cameraPosition;
    float resX;
    float resY;
    float fov;
};

struct TimeBuffer{
    float time;
    float timeOfDay;
};

// TOOLBOX
float remap(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

float saturate(float value) {
    return clamp(value, 0.0, 1.0);
}

// Fast hash function based on thread position and optional time seed
float getRandomOffset(uint3 threadId, uint frameSeed) {
    // Simple integer hash
    uint hash = threadId.x * 1597334673U + threadId.y * 3812015801U + frameSeed;
    hash = hash ^ (hash >> 16);
    hash = hash * 2246822519U;
    hash = hash ^ (hash >> 13);
    hash = hash * 3266489917U;
    hash = hash ^ (hash >> 16);
    
    // Convert to [0,1] range
    return float(hash) * (1.0 / 4294967296.0);
}

float powder(float d) {
    return (1.0 - exp(-2.0 * d));
}

float3 computeClipSpaceCoord(uint2 fragCoord, int2 res) {
    float2 rayNds = 2.0 * float2(fragCoord.xy) / float2(res) - 1.0;
    return float3(rayNds, 1.0);
}

// Calculate fog factor based on distance
float3 applyAerialPerspective(float3 pixelColor, float3 backgroundColor, float distance, float maxVisibilityDistance, float densityFactor) {
    // This is an exponential fog model which is more realistic than linear
    float fogFactor = 1.0 - exp(-distance * densityFactor);
    
    // Clamp fog factor
    fogFactor = saturate(fogFactor);
    
    // Optional: Adjust fog color based on altitude if you have that information
    float3 fogColor = backgroundColor;
    
    // Blend between pixel color and fog color
    return lerp(pixelColor, fogColor, fogFactor);
}

// PHASE FUNCTIONS
float henyeyGreenstein(float sundotrd, float g) {
    float gg = g * g;
    return (1.0 - gg) / pow(1.0 + gg - 2.0 * g * sundotrd, 1.5);
}

float dualLobePhase(float sundotrd, float phaseG) {
    return lerp(henyeyGreenstein(sundotrd, -phaseG), henyeyGreenstein(sundotrd, phaseG), saturate(sundotrd * 0.5 + 0.5));
}

float henyeyGreensteinModified(float sundotrd, float ecc) {
    return ((1.0 - ecc * ecc) / pow((1.0 + ecc * ecc - 2.0 * ecc * sundotrd), 1.5)) / (4.0 * PI);
}

float directedPhase(float sundotrd, float eccentricity, float silverIntensity, float silverSpread) {
    return max(henyeyGreensteinModified(sundotrd, eccentricity), silverIntensity * henyeyGreensteinModified(sundotrd, 0.99 - silverSpread));
}

float isophase() {
    return 1.0 / (4.0 * PI);
}

// RAYCASTING
float2 intersectRayAABB(float3 rayOrigin, float3 rayDir, float3 aabbMin, float3 aabbMax) {
    float3 tMin = (aabbMin - rayOrigin) / rayDir;
    float3 tMax = (aabbMax - rayOrigin) / rayDir;
    float3 t1 = min(tMin, tMax);
    float3 t2 = max(tMin, tMax);
    float tEnter = max(t1.x, max(t1.y, t1.z));
    float tExit = min(t2.x, min(t2.y, t2.z));
    if (tExit < 0.0 || tEnter > tExit) return float2(-1.0);
    return float2(tEnter, tExit);
}

float2 intersectRaySphere(float3 center, float radius, float3 origin, float3 direction) {
    float3 offset = origin - center;
    const float a = 1.0;
    float b = 2.0 * dot(offset, direction);
    float c = dot(offset, offset) - radius * radius;

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0) {
        float s = sqrt(discriminant);
        float dstToSphereNear = max(0.0, (-b - s) / (2.0 * a));
        float dstToSphereFar = (-b + s) / (2.0 * a);

        if (dstToSphereFar >= 0.0) {
            return float2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }
    return float2(0.0, 0.0);
}
