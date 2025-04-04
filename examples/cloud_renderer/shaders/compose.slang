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

float3 reconstructWorldPosition(float2 uv, float depth, float4x4 invProj, float4x4 invView) {
    // Convert UV to clip space (NDC: -1 to 1 range)
    float2 clipSpace = uv * 2.0 - 1.0;

    // Reconstruct clip space position (homogeneous coordinates)
    float4 clipPosition = float4(clipSpace, depth, 1.0);

    // Transform to view space
    float4 viewPosition = mul(invProj, clipPosition);
    viewPosition.xyz /= viewPosition.w; // Perspective divide

    // Transform to world space
    float4 worldPosition = mul(invView, viewPosition);

    return worldPosition.xyz;
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


    // Sample sky color
    float4 skyColor = skyColorImage.SampleLevel(uv, 0.0);

    // Blend
    float3 color = blend(skyColor.rgb, cloudIntensity, cloudAlpha);
    return color;
}

float4 terrainColor(float2 uv){
    // Sample the terrain
    float4 terrainColor = terrainColorImage.SampleLevel(float2(uv.x, uv.y), 0.0);
    float3 color = terrainColor.rgb;


    if (terrainColor.a == 1.0){
        // Sample the depth buffer
        float depth = terrainDepthImage.SampleLevel(float2(uv.x, uv.y), 0.0).r;

        // Compute terrain world position
        float3 terrainWorldPos = reconstructWorldPosition(uv, depth, data.inverseProjection, data.inverseView);

         // Compute terrain altitude (height above ground)
        float terrainAltitude = terrainWorldPos.y;

        // Sample aerial perspective LUT (assume Z is depth remapped for the LUT)
        float4 ap = aerialLUT.SampleLevel(float3(uv, depth), 0.0);
        float3 inScatter = ap.xyz; // Atmospheric in-scattering
        float eyeTransmittance = ap.w;

        // Compute sun angle for transmittance LUT
        float3 sunTransmittance = getTransmittance(transmittanceLUT, terrainAltitude, asin(data.sunDirection.y));

        float4 shadowClip = mul(float4(terrainWorldPos, 1), data.shadowViewProj);
        float2 shadowNDC = shadowClip.xy / shadowClip.w;
        float2 shadowUV = 0.5 + float2(0.5, -0.5) * shadowNDC;

        float shadowFactor = 1;
        if(all(saturate(shadowUV) == shadowUV))
        {
            float shadedDepth = shadowClip.z;
            float sampledDepth = sunShadow.SampleLevel(shadowUV, 0.0).r;
            shadowFactor = shadedDepth <= sampledDepth;
        }

        color *= shadowFactor * sunTransmittance * eyeTransmittance;
        color += inScatter;
    }

    return float4(color, terrainColor.a);
}



[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord: SV_Position) {
    PixelOutput output;

    float4 terrainColorSample = terrainColor(float2(input.uv.x, 1.0 - input.uv.y));
    output.color = terrainColorSample;
    
    if(terrainColorSample.a < 1.0){
        output.color = float4(skyColor(float2(input.uv.x, 1.0 - input.uv.y)), 1.0);
    }

    
    return output;
}