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

struct DepthData {
    float4x4 ViewProjectionMatrix;
};

[vk::push_constant] DepthData data ;

[shader("vertex")]
VSOutput vertexMain(VSInput input) {
    VSOutput output;
    output.position = mul(data.ViewProjectionMatrix, float4(input.position, 1.0));
    output.normal = input.normal;
    output.uv = input.uv;
    output.tangent = input.tangent;
    return output;
}

[shader("pixel")]
void pixelDepth(VSOutput input, out float depth : SV_Depth) {
    
}


[shader("pixel")]
void pixelLinearDepth(VSOutput input, out float depth : SV_Depth) {
    // Manually set depth to the view-space Z value
    float viewSpaceZ = input.position.z;  // This is the Z value in view space (camera space)

    // Manually write to the depth buffer using SV_Depth
    depth = viewSpaceZ;
}
