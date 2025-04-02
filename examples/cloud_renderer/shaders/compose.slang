import toolbox;

// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct CompositionBuffer{
    float4x4 inverseView;
    float4x4 inverseProjection;
    float4 frustumA;
    float4 frustumB;
    float4 frustumC;
    float4 frustumD;
    float resX;
    float resY;
}

[vk::binding(0)] ConstantBuffer<CompositionBuffer> data;
[vk::binding(1)] Sampler2D<float4> terrainColorImage;
[vk::binding(2)] Sampler2D<float> terrainDepthImage;
[vk::binding(3)] Sampler2D<float4> cloudsColorImage;
[vk::binding(4)] Sampler2D<float4> skyColorImage;


// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}


float3 blendScreen(float3 base, float3 blend) {
    return 1.0 - (1.0 - base) * (1.0 - blend);
}

float3 blendSoftLight(float3 base, float3 blend) {
    return (max(blend.x,max(blend.y, blend.z)) < 0.5) ?
    base - (1.0 - 2.0 * blend) * base * (1.0 - base) :
    base + (2.0 * blend - 1.0) * (sqrt(base) - base);
}

float3 blend(float3 base, float3 blend, float alpha){
    return base * (1.0 - alpha) + blend;
}


[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord: SV_Position) {
    PixelOutput output;

    float4 skyColor = skyColorImage.SampleLevel(float2(input.uv.x, 1.0 - input.uv.y), 0.0);
    float4 invSkyColor = skyColorImage.SampleLevel(float2(input.uv.x, input.uv.y), 0.0);

    //float4 cloudColor = cloudsColorImage.SampleLevel(float2(input.uv.x, 1.0 - input.uv.y), 0.0);
    //output.color = float4(skyColor.rgb * (1.0 - cloudColor.a) + cloudColor.rrr, 1.0);
    //return output;
    
    // Sample the terrain
    float4 terrainColor = terrainColorImage.SampleLevel(float2(input.uv.x, 1.0 - input.uv.y), 0.0);

    output.color = terrainColor;
    if(terrainColor.a < 1.0){
        float4 cloudColor = cloudsColorImage.SampleLevel(float2(input.uv.x, 1.0 - input.uv.y), 0.0);
        output.color = float4(skyColor.rgb * (1.0 - cloudColor.a) + cloudColor.rrr, 1.0);
    }
    
    return output;
}