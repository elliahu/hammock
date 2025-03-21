struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct PostProcessingData{
    float4 colorTint;
    float exposure;
    float gamma;
    int tonemapOperator;
    float contrast;
    float brightness;
    float saturation;
    float vignetteStrength;
    float vignetteSoftness;
    float temperature ;
    float grainAmount;
    int cloudBlendMode ;
    int rayBlendMode;
    float time;
    int displayShadowmap;
};

[vk::binding(0)] Sampler2D compositionImage;
[vk::push_constant] PostProcessingData post;

// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position){
    PixelOutput output;

    output.color = compositionImage.SampleLevel(input.uv, 0.0);

    return output;
}

