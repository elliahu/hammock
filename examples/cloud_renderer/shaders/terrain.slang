import toolbox;

// Input and output structures
struct VertexInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float4 tangent  : TANGENT;
};

struct VertexOutput {
    float2 uv;
    float3 worldNormal;
    float4 position : SV_Position;
    float4 tangent;
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct TerrainData{
    float4x4 modelViewProj;
};

/*
struct GlobalData{
    float4x4 invView;
    float4x4 invProj;
    float4x4 view;
    float4x4 proj;
    float4 cameraPosition;
    float4 lightColor; // W is strength
    float4 lightDirection;
    float4 skyColorZenith;
    float4 skyColorHorizon;
    float4 windDirection;
    float resX;
    float resY;
    float fov;
    float time = 0.0f;
    float timeOfDay = 0.0f;
};
*/

[vk::binding(0,0)] ConstantBuffer<GlobalData> globalData;
[vk::push_constant] TerrainData terrainData;


// Vertex Shader
[shader("vertex")]
VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;

    // This is so that slang compiler does not optimize out the tangent as it is not really used here, but the engine passes it anyway
    // This way we do not get the performance warning
    output.tangent = input.tangent;

    // Transform the position to clip space
    float4 worldPosition = float4(input.position, 1.0);
    output.position = mul(terrainData.modelViewProj, worldPosition);

    // Pass normal in world space (or object space if no model matrix is applied)
    output.worldNormal = normalize(input.normal);

    // Pass through UVs
    output.uv = input.uv;

    return output;
}

// Pixel Shader
[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position) {
    PixelOutput output;

    // Lambert shading
    float3 lightDir = normalize(globalData.lightDirection.xyz);
    float NdotL = max(dot(input.worldNormal, lightDir), 0.0);

    float3 lightColor = globalData.lightColor.rgb;
    float3 brownColor = float3(72.0/255.0, 72.0/255.0, 58.0/255.0); 

    float3 diffuse = brownColor * lightColor * NdotL;

    output.color = float4(diffuse, 1.0);

    return output;
}