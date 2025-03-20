// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

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

[vk::binding(0)] Sampler2D godRaysMaskTexture;
[vk::push_constant] BlurData data;

// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID: SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

float rand2d(float2 co) {
    return fract(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

float3 applyRadialBlur(float2 coord, float2 uv, int numSamples, float density, float exposure, float decay, float weight, float2 screenSpaceSunPos){
    // Calculate the step vector (delta) from the pixel toward the sun
    float2 deltaCoords = (screenSpaceSunPos - uv) * density / float(numSamples);

    float2 sampleUV = uv;
    float3 colorAccum = godRaysMaskTexture.SampleLevel(sampleUV, 0.0).rgb;
    float currentDecay = 1.0;

    // Accumulate samples along the ray toward the sun
    for (int i = 0; i < numSamples; ++i) {
        sampleUV += deltaCoords;
        float3 sample = godRaysMaskTexture.SampleLevel(sampleUV, 0.0).rgb;
        colorAccum += sample * currentDecay * weight;
        currentDecay *= decay;
    }

    return colorAccum * exposure;
}


[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord: SV_Position) {
    // Prep the output
    PixelOutput output;

    // Apply the blur
    float3 blur = applyRadialBlur(input.position.xy, input.uv, data.numSamples, data.density, data.exposure, data.decay, data.weight, data.screenSpaceLightPos.xy);

    // Calculate fade factor based on how far the sun is from the center
    float2 screenCenter = float2(0.5, 0.0);
    float dist = length(data.screenSpaceLightPos.xy - screenCenter);
    float fadeFactor = saturate(1.0 - (dist / data.activeDistance));

    // Final color
    output.color = float4(blur, data.alpha * fadeFactor);
    return output;
}