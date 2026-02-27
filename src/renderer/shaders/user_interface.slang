// ui.slang
// Compile with:
//   slangc ui.slang -entry vsMain -stage vertex   -target spirv -o ui.vert.spv
//   slangc ui.slang -entry fsMain -stage fragment -target spirv -o ui.frag.spv

// ─── Push constants ───────────────────────────────────────────────────────────

struct PushConstants {
    float2 screenSize;
};

[[vk::push_constant]]
PushConstants pc;

// ─── Font atlas sampler ───────────────────────────────────────────────────────

[[vk::binding(0, 0)]]
Sampler2D fontAtlas;

// ─── Vertex input ─────────────────────────────────────────────────────────────

struct VertexInput {
    [[vk::location(0)]] float2 position   : POSITION;
    [[vk::location(1)]] float2 uv         : TEXCOORD0;
    [[vk::location(2)]] float4 color      : COLOR;
    [[vk::location(3)]] float  useTexture : TEXCOORD1;
};

// ─── Vertex → fragment ────────────────────────────────────────────────────────

struct VertexOutput {
    float4 position   : SV_Position;
    float2 uv         : TEXCOORD0;
    float4 color      : COLOR;
    float  useTexture : TEXCOORD1;
};

// ─── Vertex shader ────────────────────────────────────────────────────────────

[shader("vertex")]
VertexOutput vsMain(VertexInput IN) {
    VertexOutput OUT;

    // Convert pixel coordinates to NDC [-1, 1]
    float2 ndc = (IN.position / pc.screenSize) * 2.0 - 1.0;
    OUT.position   = float4(ndc, 0.0, 1.0);
    OUT.uv         = IN.uv;
    OUT.color      = IN.color;
    OUT.useTexture = IN.useTexture;

    return OUT;
}

// ─── Fragment shader ──────────────────────────────────────────────────────────

[shader("pixel")]
float4 fsMain(VertexOutput IN) : SV_Target {
    if (IN.useTexture > 0.5) {
        // Text glyph: atlas is R8_UNORM greyscale.
        // Use the red channel as the alpha mask, tinted by the vertex color.
        float mask = fontAtlas.Sample(IN.uv).r;
        return float4(IN.color.rgb, IN.color.a * mask);
    } else {
        // Solid rectangle
        return IN.color;
    }
}
