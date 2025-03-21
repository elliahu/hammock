struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct PostProcessingData{
    float4 colorTint;
    float exposure;
    float gamma;
    int tonemapOperator;
    float contrast;
    float brightness;
    float saturation;
    float vignetteStrength;
    float vignetteSoftness;
    float temperature ;
    float grainAmount;
    float time;
};

[vk::binding(0)] Sampler2D compositionImage;
[vk::push_constant] PostProcessingData post;

// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

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

[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position){
    PixelOutput output;

    float3 color = compositionImage.SampleLevel(input.uv, 0.0).rgb;

    // Apply exposure
    float3 hdrColor = color * pow(2.0, post.exposure);


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

    // Output final color=
    output.color = float4(graded, 1.0);

    return output;
}

