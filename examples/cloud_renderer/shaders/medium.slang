import toolbox;

// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct UniformBuffer{
    float4x4 view;
    float4x4 proj;
    float4 eye;
    float4 lightPosition;
    float4 lightColor;
    float resX;
    float resY;
    float elapsedTime;
};

struct PushConstants {
    float4 scattering;
    float4 absorption;
    float g;
    float densityMultiplier;
    float densityScale;
    int enableJitter;
    float jitterStrenght;
    int lightSteps;
    float lightStepSize;
};

[vk::push_constant] PushConstants constants;
[vk::binding(0)] ConstantBuffer<UniformBuffer> buffer;
[vk::binding(1)] Sampler3D sdfTexture;
[vk::binding(2)] Sampler3D densityNoiseTexture;
[vk::binding(3)] Sampler2D curlTexture; // TODO remove, unused
[vk::binding(4)] Sampler2D blueNoiseTexture;

#define BACKGROUND float3(0.15)

#define NUM_STEPS 128
#define NUM_LIGHT_STEPS constants.lightSteps
#define LIGHT_STEP_SIZE constants.lightStepSize

#define AABB_MIN float3(- 0.5)
#define AABB_MAX float3(0.5)

#define LIGHT_COLOR buffer.lightColor.rgb

// Scattering parameters
#define SCATTERING constants.scattering.xyz
#define ABSORPTION constants.absorption.xyz
#define PHASE_G constants.g


// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;

    // Replicate the GLSL trick:
    //   outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);

    // Transform into clip space for a big triangle covering the screen
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);

    // If you need [0..1] UVs in the pixel shader, then do:
    //output.uv *= 0.5;

    return output;
}

// Convert world position to AABB UVW
float3 worldToAABB(float3 worldPos) {
    return (worldPos - AABB_MIN) / (AABB_MAX - AABB_MIN);
}

// Sample the signed distance field
float sdf(float3 p) {
    float3 uvw = worldToAABB(p);
    uvw = clamp(uvw, float3(0.001), float3(0.999));
    return sdfTexture.SampleLevel(uvw, 0.0).r;
}

// Density sampling function
float sampleDensity(float3 p) {
    float dist = sdf(p);
    if (dist > 0.0) return 0.0;

    float3 uvw = worldToAABB(p) * constants.densityScale;
    float4 density = densityNoiseTexture.SampleLevel(uvw, 0.0);
    float fbm = dot(density.gba, float3(0.625, 0.25, 0.125));
    return remap(density.r, -(1.0 - fbm), 1.0, 0.0, 1.0) * constants.densityMultiplier;
}

// Light ray attenuation computation
float3 lightRayAttenuation(float3 p){
    float3 dirToLight = normalize(buffer.lightPosition.xyz);
    float3 attenuationAlongLightRay = float3(1.0);

    // Lightmarch
    for(int l = 0; l < NUM_LIGHT_STEPS; l++){
        float3 pos = p + dirToLight * l * LIGHT_STEP_SIZE;
        float density = sampleDensity(pos);
        if(density > 0.0){
            float3 attenuation = exp(-density * LIGHT_STEP_SIZE * (SCATTERING + ABSORPTION));
            attenuationAlongLightRay *= attenuation;
        }

        if (max(attenuationAlongLightRay.r, max(attenuationAlongLightRay.g, attenuationAlongLightRay.b)) < 0.01) break;
    }

    return attenuationAlongLightRay;
}


[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position){
    // Prep the output
    PixelOutput output;

    // Simpler ray direction calculation for testing
    float2 ndc = (input.position.xy / float2(buffer.resX, buffer.resY)) * 2.0 - 1.0;
    float4 rayClip = float4(ndc, 1.0, 1.0);
    float4 rayEye = mul(inverse(buffer.proj), rayClip);
    rayEye.w = 0.0; // Direction vector
    float3 rayDirection = normalize(mul(inverse(buffer.view), rayEye).xyz);
    float3 rayOrigin = buffer.eye.xyz;

    // Intersection
    float2 intersection = intersectRayAABB(rayOrigin, rayDirection, AABB_MIN, AABB_MAX);
    if (intersection.y < 0.0) {
        output.color = float4(BACKGROUND, 1.0);
        return output;
    }

    // Ray marching variables
    float stepSize = (intersection.y - intersection.x) / float(NUM_STEPS);
    // Get a blue noise sample; ideally, normalize your coordinates
    float3 noise = (constants.enableJitter == 1) ? blueNoiseTexture.SampleLevel(input.uv * 10, 0.0).rgb * constants.jitterStrenght : float3(0.0);
    // Use one channel (e.g., the red channel) to determine an offset within one step interval
    float jitterOffset = noise.r * stepSize;
    // Apply the jitter to the start position
    float3 p = rayOrigin + rayDirection * (intersection.x + jitterOffset);
    // Acumulated values
    float3 totalTransmittance = float3(1.0);
    float3 inScattering = float3(0.0);

     // Raymarch
    for (int i = 0; i < NUM_STEPS; i++) {
        float density = sampleDensity(p);

        if (density > 0.0) {
            // Compute the light contribution along the light ray from this point
            float3 attenuationAlongLightRay = lightRayAttenuation(p);
            float3 lightContribution = LIGHT_COLOR * attenuationAlongLightRay;

            // Calculate phase using the angle between the light and view direction.
            float3 L = normalize(buffer.lightPosition.xyz - p);  // direction from point p to the light
            float3 V = normalize(rayDirection);          // direction from point p to the camera
            float cosTheta = dot(L, V);
            float phase = henyeyGreenstein(cosTheta, PHASE_G);

            // Compute the differential in-scattering contribution using the current transmittance
            float3 dL = totalTransmittance * density * SCATTERING * phase * lightContribution  * stepSize;
            inScattering += dL;

            // Now update total transmittance for extinction along the ray segment
            float3 attenuation = exp(-density* stepSize* (SCATTERING + ABSORPTION));
            totalTransmittance *= attenuation;
        }

        if (max(totalTransmittance.r, max(totalTransmittance.g, totalTransmittance.b)) < 0.01)
        break;

        p += rayDirection * stepSize;
    }

    // Optionally, compute alpha from the accumulated optical depth,
    // but here we base it on the remaining transmittance.
    float alpha = 1.0 - max(totalTransmittance.r, max(totalTransmittance.g, totalTransmittance.b));

    // Use the in-scattered radiance as the final color output.
    float3 color = inScattering;

    output.color = float4(color, alpha);

    if(alpha == 0.0){
        output.color = float4(BACKGROUND, 1.0);
    }

    return output;
}