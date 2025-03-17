import toolbox;

// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct Set{
    ConstantBuffer<CameraBuffer> cameraBuffer;
    ConstantBuffer<TimeBuffer> timeBuffer;
    ConstantBuffer<SunAndSkyBuffer> sunAndSkyBuffer;
};

ParameterBlock<Set> set;



// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;

    // Replicate the GLSL trick:
    //   outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);

    // Transform into clip space for a big triangle covering the screen
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);

    // If you need [0..1] UVs in the pixel shader, then do:
    //output.uv *= 0.5;

    return output;
}

[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position){
    // Prep the output
    PixelOutput output;

    // Simpler ray direction calculation for testing
    float2 ndc = (input.position.xy / float2(set.cameraBuffer.resX, set.cameraBuffer.resY)) * 2.0 - 1.0;
    float4 rayClip = float4(ndc, 1.0, 1.0);
    float4 rayEye = mul(set.cameraBuffer.inv_proj, rayClip);
    rayEye.w = 0.0; // Direction vector
    float3 rayDirection = normalize(mul(set.cameraBuffer.inv_view, rayEye).xyz);

    //Base sky color
    float3 skyColorZenith = set.sunAndSkyBuffer.skyColorZenith.rgb;
    float3 skyColorHorizon = set.sunAndSkyBuffer.skyColorHorizon.rgb;
    float3 skyColorNadir = float3(40.0/255.0,48.0/255.0, 64.0/255.0);

    // Calculate the angle between the ray direction and the up vector (y-axis)
    // Up vector is (0, 1, 0) in world space
    float upDot = dot(rayDirection, float3(0, 1, 0));

    float horizonOffset = -0.02;
    
    // Map the dot product to create a smooth gradient
    float3 finalColor;
    
    if (upDot > horizonOffset) {
        // Above horizon (between zenith and horizon)
        // Remap upDot from [0, 1] to [0, 1] for blend factor
        float t = 1.0 - ((upDot - horizonOffset) / (1.0 - horizonOffset)); // Invert so 1 = horizon, 0 = zenith
        // Smooth transition using power function for more natural look
        t = pow(t, 5.0); // Adjust exponent as needed for desired curve
        finalColor = lerp(skyColorZenith, skyColorHorizon, t);
    } else {
        // Below horizon (between horizon and nadir)
        // Remap upDot from [-1, 0] to [0, 1] for blend factor
        float t = (-upDot + horizonOffset) / (1.0 + horizonOffset); // Map -1...0 to 1...0
        // Swift but visible transition
        t = pow(t, .5); // Higher power creates swifter transition
        finalColor = lerp(skyColorHorizon, skyColorNadir, t);
    }
    
    // Set the output color
    output.color = float4(finalColor, 1.0);
    
    return output;
}


