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

    // Compute ray direction in world space
    float4 rayClip = float4(computeClipSpaceCoord(uint2(input.uv.xy), int2(set.cameraBuffer.resX, set.cameraBuffer.resY)), 1.0);
    float4 rayView = mul(set.cameraBuffer.inv_proj, rayClip);
    rayView = float4(rayView.xy, -1.0, 1.0);
    float3 rayDirection = normalize(mul(set.cameraBuffer.inv_view, rayView).xyz);

    //Base sky color
    float3 skyColor = set.sunAndSkyBuffer.skyColor.rgb;

    // Sun disk params
    const float sunHalfAngle = 2.5 * PI / 180.0;

    // Get the actual sun direction
    float3 sunDirection = normalize(set.sunAndSkyBuffer.lightDirection.xyz);

    // Compute an effective sun direction for drawing the disk.
    float3 effectiveSunDir = sunDirection;
    if (sunDirection.y < 0.0) {
        effectiveSunDir.y = 0.0;
        effectiveSunDir = normalize(effectiveSunDir);
    }

    // Compute the angular difference between the current ray and the effective sun direction.
    float dotSun = dot(rayDirection, effectiveSunDir);
    float angleDiff = acos(clamp(dotSun, -1.0, 1.0));
    
    // Use smoothstep to create a soft-edged disk.
    float sunMask = 1.0 - smoothstep(sunHalfAngle * 0.9, sunHalfAngle, angleDiff);

    // Also, clip out fragments that are below the horizon.
    float horizonMask = step(0.0, rayDirection.y);
    float finalSunMask = sunMask * horizonMask;

    // Blend the sun color (lightColor.rgb) over the sky.
    float3 finalColor = lerp(skyColor, set.sunAndSkyBuffer.lightColor.rgb, finalSunMask);

    output.color = float4(finalColor, 1.0);
    return output;
}


