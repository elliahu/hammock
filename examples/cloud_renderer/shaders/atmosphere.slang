#pragma once

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
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    // Map the vertexID (0, 1, 2, 3) to the four corners of a fullscreen quad
    VertexOutput output;
    
    // Define the coordinates of a fullscreen quad in NDC (-1 to 1)
    if (vertexID == 0) {
        output.position = float4(-1.0, -1.0, 0.0, 1.0); // Bottom-left
        output.uv = float2(0.0, 1.0); // Bottom-left UV
    } else if (vertexID == 1) {
        output.position = float4(1.0, -1.0, 0.0, 1.0); // Bottom-right
        output.uv = float2(1.0, 1.0); // Bottom-right UV
    } else if (vertexID == 2) {
        output.position = float4(-1.0, 1.0, 0.0, 1.0); // Top-left
        output.uv = float2(0.0, 0.0); // Top-left UV
    } else {
        output.position = float4(1.0, 1.0, 0.0, 1.0); // Top-right
        output.uv = float2(1.0, 0.0); // Top-right UV
    }
    
    return output;
}

[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position){
    // Prep the output
    PixelOutput output;

    // Compute ray direction in world space
    float4 rayClip = float4(computeClipSpaceCoord(uint2(fragCoord.xy), int2(set.cameraBuffer.resX, set.cameraBuffer.resY)), 1.0);
    float4 rayView = mul(set.cameraBuffer.inv_proj, rayClip);
    rayView = float4(rayView.xy, -1.0, 1.0);
    float3 rayDirection = normalize(mul(set.cameraBuffer.inv_view, rayView).xyz);

    //Base sky color
    float3 skyColor = float3(59.0/255.0, 110.0/255.0, 219.0/255.0);

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


