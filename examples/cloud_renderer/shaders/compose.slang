// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};


[vk::binding(0)] Sampler2D<float4> terrainColorImage;
[vk::binding(1)] Sampler2D<float> terrainDepthImage;




// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord: SV_Position) {
    PixelOutput output;
    
    // Sample values from the textures
    float4 terrainColor = terrainColorImage.SampleLevel(float2(input.uv.x, 1.0 - input.uv.y), 0.0);
    output.color = terrainColor;

    return output;
}