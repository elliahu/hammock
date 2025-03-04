#version 450

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 outColor;

//layout (binding = 3) uniform sampler2D skyDomeSampler;

layout (binding = 2) uniform SunAndSkyUBO {
    vec4 cloudColorTop;
    vec4 cloudColorBottom;
    vec4 lightColor;
    vec4 lightDirection;
    vec4 skyColorBottom;
    vec4 skyColorTop;
};

layout (binding = 0) uniform CameraUBO
{
    mat4 inv_view;
    mat4 inv_proj;
    mat4 invViewProj;
    vec4 cameraPosition;
    float resX;
    float resY;
    float fov;
};

layout (binding = 1) uniform TimeUBO
{
    float iTime;
    float timeOfDay;
};

layout (push_constant) uniform PushConstants {
    float coverage_multiplier;
    float coverageRepeat;
    float base_multiplier;
    float detail_multiplier;
    float cloudSpeed;
    float crispiness; // .4
    float curliness;
    float absorption; //0.0035
    float densityFactor; //  0.02;
    int enablePowder; // 0
    float fogFactor;
    float earthRadius;
    float sphereInnerRadius;
    float sphereOuterRadius;
    float phaseG;
    float eccentricity;
    float silverIntensity;
    float silverSpread;
    float ambientStrength;
    float cloudTypeOverride;
    float atmosphereScatteringStrength;
    int DEBUG_cloudmap;
};

#define EARTH_RADIUS earthRadius
#define SPHERE_INNER_RADIUS (EARTH_RADIUS + sphereInnerRadius)
#define SPHERE_OUTER_RADIUS (SPHERE_INNER_RADIUS + sphereOuterRadius)
#define SPHERE_DELTA float(SPHERE_OUTER_RADIUS - SPHERE_INNER_RADIUS)

#define SUN_DIR normalize(lightDirection.xyz)
#define SUN_COLOR lightColor.rgb * vec3(1.1, 1.1, 0.95)
vec3 sphereCenter = vec3(0.0, -EARTH_RADIUS, 0.0);

const float PI = 3.14159265359;

#define DENSITY_FALLOFF 4.0
#define OPTICAL_DEPTH_POINTS 6
#define ATMOSPHERE_SAMPLE_POINTS 6

const vec3 wavelengths = vec3(700.0, 530.0, 440.0);

#define SCATTER_R pow(20.0/wavelengths.r, 4) * atmosphereScatteringStrength
#define SCATTER_G pow(20.0/wavelengths.g, 4) * atmosphereScatteringStrength
#define SCATTER_B pow(20.0/wavelengths.b, 4) * atmosphereScatteringStrength
#define SCATTER_COEFS vec3(SCATTER_R, SCATTER_G, SCATTER_B)

float densityAtPoint(vec3 densitySamplePoint) {
    float heightAboveSurface = length(densitySamplePoint - sphereCenter) - EARTH_RADIUS;
    float height01 = heightAboveSurface / (SPHERE_OUTER_RADIUS - EARTH_RADIUS);
    float localDensity = exp(-height01 * DENSITY_FALLOFF) * (1.0 - height01);
    return localDensity;
}

float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength) {
    vec3 densitySamplePoint = rayOrigin;
    float stepSize = rayLength / (OPTICAL_DEPTH_POINTS - 1);
    float opticalDepth = 0.0;

    for (int i = 0; i < OPTICAL_DEPTH_POINTS; i++) {
        float localDensity = densityAtPoint(densitySamplePoint);
        opticalDepth += localDensity * stepSize;
        densitySamplePoint += rayDir * stepSize;
    }
    return opticalDepth;
}

vec2 raySphere(vec3 center, float radius, vec3 origin, vec3 direction) {
    vec3 offset = origin - center;
    const float a = 1.0;
    float b = 2.0 * dot(offset, direction);
    float c = dot(offset, offset) - radius * radius;

    float discriminant = b * b - 4.0 * a * c;
    // No intersections: discriminant < 0
    // 1 intersection: discriminant == 0
    // 2 intersections: discriminant > 0

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

vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength) {
    vec3 inScatteredPoint = rayOrigin;
    float stepSize = rayLength / (ATMOSPHERE_SAMPLE_POINTS - 1);
    vec3 inScatteredLight = vec3(0.0);

    for (int i = 0; i < ATMOSPHERE_SAMPLE_POINTS; i++) {
        float sunRayLength = raySphere(sphereCenter, EARTH_RADIUS, inScatteredPoint, SUN_DIR).y;
        float sunRayOpticalDepth = opticalDepth(inScatteredPoint, SUN_DIR, sunRayLength);
        float viewRayOpticalDepth = opticalDepth(inScatteredPoint, -rayDir, stepSize * i);
        vec3 transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth) * SCATTER_COEFS);
        float localDensity = densityAtPoint(inScatteredPoint);

        inScatteredLight += localDensity * transmittance * SCATTER_COEFS * stepSize;
        inScatteredPoint += rayDir * stepSize;
    }

    return inScatteredLight;
}


void main() {
    vec4 direction_view = invViewProj * vec4(in_uv * 2.0 - 1.0, 1.0, 1.0);
    vec3 direction = normalize(direction_view.xyz / direction_view.w);

    // Convert 3D direction to 2D UV coordinates using equirectangular projection
    //    vec2 uv;
    //    uv.x = 0.5 + 0.5 * atan(direction.z, direction.x) / PI;
    //    uv.y = 0.5 - asin(clamp(direction.y, -1.0, 1.0)) / PI;
    //
    //
    //    // Adjust gradient to have skyColorBottom at the horizon (y = 0) and skyColorTop at the top (y = 1)
    //    float gradientFactor = clamp(direction.y * 0.5 + 0.5, 0.0, 1.0);
    //    vec3 skyColor = mix(skyColorBottom.rgb, skyColorTop.rgb, gradientFactor);

    vec3 skyColor = calculateLight(cameraPosition.xyz, direction, raySphere(sphereCenter, SPHERE_OUTER_RADIUS, cameraPosition.xyz, direction).y) * SUN_COLOR;

    outColor = vec4(skyColor, 1.0);

}