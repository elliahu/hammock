// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct PostProcessingBuffer{
    float4 colorTint;

    // Tonemapping parameters
    float exposure;
    float gamma;
    int tonemapOperator;  // 0: Linear, 1: Reinhard, 2: ACES, 3: Uncharted 2

    // Color grading parameters
    float contrast;
    float brightness;
    float saturation;


    float vignetteStrength;
    float vignetteSoftness;

    // Color temperature/white balance
    float temperature; // -1.0 to 1.0, cool to warm

    // Film grain
    float grainAmount;

    // Adjust cloud blending
    int cloudBlendMode; // 0: Normal, 1: Screen, 2: Soft Light

    float time;
};

[vk::binding(0)] Sampler2D cloudTexture;
[vk::binding(1)] Sampler2D godRaysImageTexture;
[vk::binding(2)] Sampler2D atmosphereTexture;
[vk::binding(3)] Sampler2D shadowMap;
[vk::binding(4)] ConstantBuffer<PostProcessingBuffer> post;


// Utility functions
float3 rgb2hsv(float3 c) {
    float4 K = float4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 hsv2rgb(float3 c) {
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Random hash function for film grain
float hash(float2 p) {
    p = 50.0 * fract(p * 0.3183099);
    return fract(p.x * p.y * (p.x + p.y));
}

// ACES Tonemapping function
float3 ACESFilm(float3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Uncharted 2 tonemapping (modified version)
float3 Uncharted2Tonemap(float3 color) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

// Color temperature adjustment
float3 adjustTemperature(float3 rgb, float temperature) {
    // Apply temperature shift (blue to orange)
    float3 warm = float3(1.0, 0.6, 0.4); // Warm color bias
    float3 cool = float3(0.4, 0.6, 1.0); // Cool color bias

    float3 tempAdjust = lerp(cool, warm, (temperature + 1.0) * 0.5);
    return rgb * tempAdjust;
}

// Apply vignette effect
float vignette(float2 uv, float strength, float softness) {
    uv = uv * 2.0 - 1.0;
    float dist = length(uv);
    return smoothstep(strength, strength - softness, dist);
}

// Cloud blending modes
float3 blendNormal(float3 base, float3 blend, float opacity) {
    return lerp(base, blend, opacity);
}

float3 blendScreen(float3 base, float3 blend) {
    return 1.0 - (1.0 - base) * (1.0 - blend);
}

float3 blendSoftLight(float3 base, float3 blend) {
    return (max(blend.x,max(blend.y, blend.z)) < 0.5) ?
    base - (1.0 - 2.0 * blend) * base * (1.0 - base) :
    base + (2.0 * blend - 1.0) * (sqrt(base) - base);
}


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
PixelOutput pixelMain(VertexOutput input, float4 fragCoord: SV_Position) {
    // Sample clouds, sky, and god rays
    float4 clouds = cloudTexture.SampleLevel(float2(input.uv.x, 1.0 - input.uv.y), 0.0);
    float4 sky = atmosphereTexture.SampleLevel(float2(input.uv.x, 1.0 - input.uv.y), 0.0);
    float4 godRays = godRaysImageTexture.SampleLevel(float2(input.uv.x, 1.0 - input.uv.y), 0.0);

    sky = float4(pow(sky.xyz, float3(2.2)), sky.a);

    // Cloud blending based on selected mode
    float3 cloudBlended;
    if (post.cloudBlendMode < 0.5) {
        // Normal blending
        cloudBlended = sky.rgb * (1.0 - clouds.a) + clouds.rgb;
    } else if (post.cloudBlendMode < 1.5) {
        // Screen blending
        cloudBlended = lerp(sky.rgb, blendScreen(sky.rgb, clouds.rgb), clouds.a);
    } else {
        // Soft light blending
        cloudBlended = lerp(sky.rgb, blendSoftLight(sky.rgb, clouds.rgb), clouds.a);
    }

    // Apply god rays using screen blending
    // TODO
    cloudBlended = lerp(cloudBlended.rgb, blendScreen(cloudBlended.rgb, godRays.rgb), godRays.a);

    // Apply exposure
    float3 hdrColor = cloudBlended * pow(2.0, post.exposure);


    // Apply tonemapping based on selected operator
    float3 tonemapped;
    if (post.tonemapOperator < 0.5) {
        // Linear
        tonemapped = hdrColor;
    } else if (post.tonemapOperator < 1.5) {
        // Reinhard
        tonemapped = hdrColor / (1.0 + hdrColor);
    } else if (post.tonemapOperator < 2.5) {
        // ACES
        tonemapped = ACESFilm(hdrColor);
    } else {
        // Uncharted 2
        float W = 11.2;
        float3 whiteScale = 1.0 / Uncharted2Tonemap(float3(W));
        tonemapped = Uncharted2Tonemap(hdrColor) * whiteScale;
    }

    // Apply color grading

    // Convert to HSV for easier color adjustments
    float3 hsv = rgb2hsv(tonemapped);

    // Adjust saturation
    hsv.y *= post.saturation;

    // Back to RGB
    float3 graded = hsv2rgb(hsv);

    // Apply contrast and brightness
    graded = (graded - 0.5) * post.contrast + 0.5 + post.brightness;

    // Apply color tint
    graded *= post.colorTint.rgb;

    // Apply temperature adjustment
    graded = adjustTemperature(graded, post.temperature);


    // Apply vignette
    float vig = vignette(input.uv, post.vignetteStrength, post.vignetteSoftness);
    graded *= vig;

    // Apply film grain
    if (post.grainAmount > 0.0) {
        float grain = hash((input.uv * float2(1920,1080)) + post.time * 10.0);
        graded += (grain - 0.5) * post.grainAmount;
    }

    // Apply gamma correction
    graded = pow(graded, float3(1.0 / post.gamma));

    // Output final color
    PixelOutput output;
    output.color = float4(graded, 1.0);

    // Define shadow map preview region in UV space (top-right corner)
    float2 previewSize = float2(0.2, 0.2); // 20% of the screen
    float2 previewPos = float2(1.0 - previewSize.x, 0.0); // Top-right corner (origin at top-left)

    // Check if current UV is within the preview rectangle
    bool inPreviewRegion = all(input.uv >= previewPos) && all(input.uv <= previewPos + previewSize);

    if (inPreviewRegion) {
        // Calculate UV within the preview region
        float2 localUV = (input.uv - previewPos) / previewSize;

        // Sample shadow map using flipped Y to match screen orientation
        float shadowValue = shadowMap.SampleLevel(localUV, 0.0).r;

        // Show it as grayscale (optional: enhance contrast)
        float3 shadowColor = float3(shadowValue, shadowValue, shadowValue);

        output.color = float4(shadowColor, 1.0);
    }

    return output;
}