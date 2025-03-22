struct BlurData {
    float4 screenSpaceLightPos;
    int numSamples;
    float density;
    float exposure;
    float decay;
    float weight;
    float alpha;
    float activeDistance;
};

[vk::binding(0)] RWTexture2D occlusionMask;
[vk::binding(1)] RWTexture2D godRaysImage;
[vk::push_constant] BlurData data;


float rand2d(float2 co) {
    return fract(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

// Radial Blur Function (updated to use Load)
float3 applyRadialBlur(uint2 pixelCoord, float2 uv, int numSamples, float density, float exposure, float decay, float weight, float2 screenSpaceSunPos, uint2 imageSize){
    float2 deltaCoords = (screenSpaceSunPos - uv) * density / float(numSamples);
    float2 sampleUV = uv;
    float3 colorAccum = occlusionMask.Load(int2(pixelCoord)).rgb;
    float currentDecay = 1.0;

    for (int i = 0; i < numSamples; ++i) {
        sampleUV += deltaCoords;
        uint2 sampleCoord = uint2(sampleUV * float2(imageSize));
        sampleCoord = clamp(sampleCoord, uint2(0), imageSize - 1);
        float3 sample = occlusionMask.Load(int2(sampleCoord)).rgb;
        colorAccum += sample * currentDecay * weight;
        currentDecay *= decay;
    }

    return colorAccum * exposure;
}

[numthreads(16, 16, 1)]
[shader("compute")]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    uint2 imageSize;

    // Get image size (assuming both input and output have the same size)
    occlusionMask.GetDimensions(imageSize.x, imageSize.y);

    if (pixelCoord.x >= imageSize.x || pixelCoord.y >= imageSize.y)
        return; // Avoid out-of-bounds access

    // Convert pixel coordinate to UV
    float2 uv = float2(pixelCoord) / float2(imageSize);

    // Apply the radial blur
    float3 blur = applyRadialBlur(pixelCoord, uv, data.numSamples, data.density, data.exposure, data.decay, data.weight, data.screenSpaceLightPos.xy, imageSize);

    // Calculate fade factor based on sun distance from screen center
    float2 screenCenter = float2(0.5, 0.0);
    float dist = length(data.screenSpaceLightPos.xy - screenCenter);
    float fadeFactor = saturate(1.0 - (dist / data.activeDistance));

    // Final color
    float4 finalColor = float4(blur, data.alpha * fadeFactor);

    // Write to output image
    godRaysImage[pixelCoord] = finalColor;
}
