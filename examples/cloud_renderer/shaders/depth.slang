struct VSInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float4 tangent  : TANGENT;
};


struct VSOutput {
    float4 position : SV_Position;
    float3 normal;
    float2 uv;
    float4 tangent;
};

[vk::push_constant] float4x4 ViewProjectionMatrix;

[shader("vertex")]
VSOutput vertexMain(VSInput input) {
    VSOutput output;
    output.position = mul(ViewProjectionMatrix, float4(input.position, 1.0));
    output.normal = input.normal;
    output.uv = input.uv;
    output.tangent = input.tangent;
    return output;
}

[shader("pixel")]
void pixelMain(VSOutput input) {
    
}