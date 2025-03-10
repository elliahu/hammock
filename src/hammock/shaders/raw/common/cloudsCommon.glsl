#ifndef CLOUDS_COMMON
#define CLOUDS_COMMON

#define PI 3.14159265358979323846

// TOOLBOX
// Remaps value from one range to another
float remap(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

// remap clamped after
float remapc(in float value, in float original_min, in float original_max, in float new_min, in float new_max)
{
    float t = new_min + ( ((value - original_min) / (original_max - original_min)) * (new_max - new_min) );
    return clamp(t, new_min, new_max);
}

// remap clamped before and after
float remapcc(in float value, in float original_min, in float original_max, in float new_min, in float new_max)
{
    value = clamp(value, original_min, original_max);
    float t = new_min + ( ((value - original_min) / (original_max - original_min)) * (new_max - new_min) );
    return clamp(t, new_min, new_max);
}

// Swap two values
void swap(in float a, in float b) {
    float c = a;
    a = b;
    b = c;
}

// Beers-powder aproximation
float powder(float d) {
    return (1. - exp(-2. * d));
}

// TODO this can be done on CPU
vec3 getSunColor(float timeOfDay) {
    // Define key colors:
    vec3 midnightColor = vec3(0.0, 0.0, 0.2);       // dark blue at midnight
    vec3 sunriseSunsetColor =  vec3(1.0,0.5,0.3); //vec3(1.0, 0.5, 0.2);    // orange/red at sunrise/sunset
    vec3 noonColor = vec3(1.0, .9, .9);             // white at noon

    // Ensure timeOfDay is between 0 and 1
    timeOfDay = fract(timeOfDay);

    if (timeOfDay < 0.25) {
        // Transition from midnight to sunrise/sunset color
        float t = timeOfDay / 0.25;
        return mix(midnightColor, sunriseSunsetColor, smoothstep(0.0, 1.0, t));
    }
    else if (timeOfDay < 0.5) {
        // Transition from sunrise/sunset color to noon color
        float t = (timeOfDay - 0.25) / 0.25;
        return mix(sunriseSunsetColor, noonColor, smoothstep(0.0, 1.0, t));
    }
    else if (timeOfDay < 0.75) {
        // Transition from noon color back to sunrise/sunset color
        float t = (timeOfDay - 0.5) / 0.25;
        return mix(noonColor, sunriseSunsetColor, smoothstep(0.0, 1.0, t));
    }
    else {
        // Transition from sunrise/sunset color back to midnight
        float t = (timeOfDay - 0.75) / 0.25;
        return mix(sunriseSunsetColor, midnightColor, smoothstep(0.0, 1.0, t));
    }
}

vec3 computeClipSpaceCoord(uvec2 fragCoord, ivec2 res) {
    vec2 rayNds = 2.0 * vec2(fragCoord.xy) / res - 1.0;
    return vec3(rayNds, 1.0);
}


// PHASE FUNCTIONS
// mie scattering Henyey-Greenstein phase function
float henyeyGreenstein(float sundotrd, float g) {
    float gg = g * g;
    return (1. - gg) / pow(1. + gg - 2. * g * sundotrd, 1.5);
}

// Dula lobe Henyey-Greenstein phase function
float dualLobePhase(float sundotrd, float phaseG) {
    return mix(henyeyGreenstein(sundotrd, -phaseG), henyeyGreenstein(sundotrd, phaseG), clamp(sundotrd * 0.5 + 0.5, 0.0, 1.0));
}

// Modified Henyey-Greenstein phase function
float henyeyGreensteinModified(float sundotrd, float ecc){
    return ((1.0 - ecc * ecc) / pow((1.0 + ecc * ecc - 2.0 * ecc * sundotrd), 3.0 / 2.0)) / 4.0 * PI;
}

// Adjusted dual-lobe Henyey Greenstein phase function that can be artistically adjusted
float directedPhase(float sundotrd, float eccentricity, float silverIntensity, float silverSpread){
    return max(henyeyGreensteinModified(sundotrd, eccentricity), silverIntensity * henyeyGreensteinModified(sundotrd, 0.99 - silverSpread));
}

// Isotropic scattering phase function
float isophase(){
    return 1.0 / 4.0 * PI;
}

// RAYCASTING

// Ray-AABB intersection
// TODO this can be done using ray queries - will be hardware accelerated
vec2 intersectRayAABB(vec3 rayOrigin, vec3 rayDir, vec3 aabbMin, vec3 aabbMax) {
    vec3 tMin = (aabbMin - rayOrigin) / rayDir;
    vec3 tMax = (aabbMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tEnter = max(t1.x, max(t1.y, t1.z));
    float tExit = min(t2.x, min(t2.y, t2.z));
    if (tExit < 0.0 || tEnter > tExit) return vec2(-1.0);
    return vec2(tEnter, tExit);
}

// Ray-sphere intersection
// TODO this can be done using ray queries - will be hardware accelerated
vec2 intersectRaySphere(vec3 center, float radius, vec3 origin, vec3 direction) {
    vec3 offset = origin - center;
    const float a = 1.0;
    float b = 2.0 * dot(offset, direction);
    float c = dot(offset, offset) - radius * radius;

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0) {
        float s = sqrt(discriminant);
        float dstToSphereNear = max(0, (-b - s) / (2 * a));
        float dstToSphereFar = (-b + s) / (2 * a);

        if (dstToSphereFar >= 0) {
            return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }
    return vec2(0.0, 0.0);
}


#endif