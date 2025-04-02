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

[vk::binding(0,0)] ConstantBuffer<CompositionBuffer> data;
[vk::binding(0,1)] Sampler2D<float4> skyView;


// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}


float3 postProcessColor(float2 seed, float3 input)
{
    float rand = frac(sin(dot(
        seed, float2(12.9898, 78.233) * 2.0)) * 43758.5453);
    input = 255 * saturate(pow(input, 1));
    input = rand.xxx < (input - floor(input)) ? ceil(input) : floor(input);

    return input / 255;
}


[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position){
    // Prep the output
    PixelOutput output;

    float2 ndc = (input.position.xy / float2(data.resX, data.resY)) * 2.0 - 1.0;
    float4 rayClip = float4(ndc, 1.0, 1.0);
    float4 rayEye = mul(data.inverseProjection, rayClip);
    rayEye.w = 0.0; // Direction vector
    float3 dir = normalize(mul(data.inverseView, rayEye).xyz);

    float phi = atan2(dir.z, dir.x);
    float u = phi / (2 * PI);
    
    float theta = asin(dir.y);
    float v = 0.5 + 0.5 * sign(theta) * sqrt(abs(theta) / (PI / 2));

    output.color = float4(postProcessColor(input.uv,skyView.SampleLevel(float2(u, v), 0.0).rgb), 1.0);


    return output;
}