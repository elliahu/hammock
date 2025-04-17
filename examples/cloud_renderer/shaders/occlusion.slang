import toolbox;

// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

[vk::binding(0)] Sampler2D<float4> cloudsColorImage;
[vk::binding(1)] Sampler2D<float4> terrainDepthImage;

[vk::binding(0)] Sampler2D<float4> maskImage;


struct Params{
    float lssposX;
    float lssposY;
    int num_samples;
    float density;
    float exposure;
    float decay;
    float weight;
    float alpha;
};

[vk::push_constant] Params coefficients;


// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

[shader("pixel")]
PixelOutput pixelMask(VertexOutput input, float4 fragCoord : SV_Position) {
    PixelOutput output;
    
    float cloudAplha = cloudsColorImage.SampleLevel(input.uv, 0.0).a;
    float terrainAlpha = 1.0 - terrainDepthImage.SampleLevel(input.uv, 0.0).r;

    float mask = (cloudAplha > 0.0 || terrainAlpha > 0.0) ? 0.0 : 1.0;
    output.color = float4(mask, mask, mask, 1.0);

    return output;
}


float3 radialBlur(float2 vertex_tex_coordinates, float2 screen_space_position){
    float2 delta_tex_coord = (vertex_tex_coordinates - screen_space_position) * coefficients.density * (1.0 / float(coefficients.num_samples));
    float2 tex_coordinates = vertex_tex_coordinates;
    float3 color = maskImage.SampleLevel(tex_coordinates, 0.0).rgb;
    float decay = 1.0;
    for (int i = 0; i < coefficients.num_samples; ++i)
    {
        tex_coordinates -= delta_tex_coord;
        float3 current_sample = maskImage.SampleLevel(tex_coordinates, 0.0).rgb;
        current_sample *= decay * coefficients.weight;
        color += current_sample;
        decay *= coefficients.decay;
    }

    return color * coefficients.exposure;
}

[shader("pixel")]
PixelOutput pixelBlur(VertexOutput input, float4 fragCoord : SV_Position) {
    PixelOutput output;
    output.color = float4(radialBlur(input.uv, float2(coefficients.lssposX, coefficients.lssposY)), coefficients.alpha);
    return output;
}
