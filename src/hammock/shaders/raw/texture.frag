#version 450

layout (binding = 0) uniform sampler2D cloudsSampler;
layout (binding = 1) uniform sampler2D skySampler;


layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

vec4 blend(vec4 foreground, vec4 background) {
    float alpha = foreground.a + background.a * (1.0 - foreground.a);
    vec3 color = (foreground.rgb * foreground.a + background.rgb * background.a * (1.0 - foreground.a)) / max(alpha, 0.0001);
    return vec4(color, alpha);
}

void main()
{
    vec4 clouds = texture(cloudsSampler, vec2(uv.x, 1.0 - uv.y));
    vec4 sky = texture(skySampler, vec2(uv.x, 1.0 - uv.y));

    outFragColor = blend(clouds, sky);
}