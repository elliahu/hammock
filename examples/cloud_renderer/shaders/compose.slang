import toolbox;

// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

[vk::binding(0)] ConstantBuffer<CompositionBuffer> data;
[vk::binding(1)] Sampler2D<float4> terrainColorImage;
[vk::binding(2)] Sampler2D<float> terrainDepthImage;
[vk::binding(3)] Sampler2D<float4> cloudsColorImage;
[vk::binding(4)] Sampler2D<float4> transmittanceLUT;
[vk::binding(5)] Sampler3D<float4> aerialLUT;
[vk::binding(6)] Sampler2D<float4> sunShadow;
[vk::binding(7)] Sampler2D<float4> blueNoise;
[vk::binding(8)] Sampler2D<float4> skyColorImage;


// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

float3 reconstructWorldPosition(float2 uv, float depth, float4x4 invProj, float4x4 invView, float near, float far) {
    // The depth is already in linear space, directly using the depth value.
    // Reconstruct view-space Z from the linear depth value
    float zView = near + depth * (far - near);

    // Convert UV to NDC [-1, 1] space
    float2 ndc = uv * 2.0 - 1.0;

    // Reconstruct homogeneous clip-space position (Z value from depth, W = 1)
    float4 clipPos = float4(ndc, depth, 1.0);

    // Convert from clip space to view space using the inverse projection matrix
    float4 viewPos = mul(invProj, clipPos);
    viewPos /= viewPos.w;  // Perform the perspective divide

    // Convert from view space to world space using the inverse view matrix
    float4 worldPos = mul(invView, viewPos);

    return worldPos.xyz;
}


float calculateShadow(float3 worldPos) {
    float4 lightClipPos = mul(data.shadowViewProj, float4(worldPos, 1.0));
    float3 ndc = lightClipPos.xyz / lightClipPos.w;
    float2 shadowUV = ndc.xy * 0.5 + 0.5;

    if (any(shadowUV < 0.0) || any(shadowUV > 1.0))
        return 1.0;

    float shadowMapDepth = sunShadow.SampleLevel(shadowUV, 0.0).r;

    float fragmentDepthInLightSpace = lightClipPos.z / lightClipPos.w;

    // If your shadow map stores linear view-space depth:
    float bias = 0.005;
    return fragmentDepthInLightSpace - bias > shadowMapDepth ? 0.0 : 1.0;
}

float4 terrainColor(float2 uv){
    // Sample the terrain
    float4 terrainColor = terrainColorImage.SampleLevel(float2(uv.x, uv.y), 0.0);
    float3 color = terrainColor.rgb;

    if (terrainColor.a == 1.0){
        // Sample the depth buffer
        float depth = terrainDepthImage.SampleLevel(float2(uv.x, uv.y), 0.0).r;

        // Compute terrain world position
        float3 terrainWorldPos = reconstructWorldPosition(uv, depth, data.inverseProjection, data.inverseView, 0.1, 1000.0);

        // Compute terrain altitude (height above ground)
        float terrainAltitude = terrainWorldPos.y * WorldScale;

        // Sample the transmittance LUT
        float4 ap = aerialLUT.SampleLevel(float3(uv, depth * WorldScale), 0.0);
        float3 inScatter = ap.xyz; // Atmospheric in-scattering
        float eyeTransmittance = ap.w;
        float3 sunTransmittance = getTransmittance(transmittanceLUT, terrainAltitude, asin(data.sunDirection.y));

        float shadowFactor = calculateShadow(terrainWorldPos);

        color *= (max(0.25, shadowFactor)) * sunTransmittance * eyeTransmittance;
        color += inScatter;
    }

    return float4(color, terrainColor.a);
}

float3 skyColor(float2 uv){
    // Sample cloud color
    float4 cloudColor = cloudsColorImage.SampleLevel(uv, 0.0);
    // R channel stores direct light intensity
    // G channel stores ambient light intensity
    float3 directLight = cloudColor.r * data.sunColor.rgb;
    float3 ambientLight = cloudColor.g * data.ambientColor.rgb;
    float3 cloudIntensity = directLight + ambientLight;
    float cloudAlpha = cloudColor.a;
    float cloudDepth = cloudColor.b;

    // Sample sky color
    float4 skyColor = skyColorImage.SampleLevel(uv, 0.0);

    // Blend
    float3 color = blend(skyColor.rgb, cloudIntensity, cloudAlpha);

    // Reconstruct the world space position
    float3 worldPos = reconstructWorldPosition(uv, cloudDepth, data.inverseProjection, data.inverseView, 0.1, 1000.0);
    // Compute the altitude of the clouds
    float cloudAltitude = worldPos.y * WorldScale;

    // Sample the transmittance LUT
    float4 ap = aerialLUT.SampleLevel(float3(uv, cloudDepth * WorldScale), 0.0);
    float3 inScatter = ap.xyz; // Atmospheric in-scattering
    float eyeTransmittance = ap.w;
    float3 sunTransmittance = getTransmittance(transmittanceLUT, cloudAltitude, asin(data.sunDirection.y));

    color *= sunTransmittance * eyeTransmittance;
    color += inScatter;

    return color;
}

[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position) {
    PixelOutput output;

    // Normalize screen coordinates
    float2 screenUV = fragCoord.xy / float2(data.resX, data.resY);

    // Define preview size and position
    float2 previewSize = float2(0.2, 0.2); // 20% of screen
    float2 previewPos = float2(0.8, 0.0);  // Top-right corner

    bool showPreview = all(screenUV >= previewPos) && all(screenUV <= (previewPos + previewSize));
#ifndef DEBUG
    showPreview = false;
#endif

    if (showPreview) {
        // Map screenUV to shadow map UV
        float2 shadowUV = (screenUV - previewPos) / previewSize;
        float depth = sunShadow.SampleLevel(shadowUV, 0.0).r;
        output.color = float4(depth.xxx, 1.0); // grayscale
    } else {
        float4 terrainColorSample = terrainColor(input.uv);
        output.color = terrainColorSample;

        if (terrainColorSample.a < 1.0) {
            output.color = float4(skyColor(input.uv), 1.0);
        }
    }

    return output;
}