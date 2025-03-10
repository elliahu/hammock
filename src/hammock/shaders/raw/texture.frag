#version 450

layout (binding = 0) uniform sampler2D cloudsSampler;
layout (binding = 1) uniform sampler2D skySampler;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec4 clouds = texture(cloudsSampler, vec2(uv.x, 1.0 - uv.y));
    vec4 sky = texture(skySampler, vec2(uv.x, 1.0 - uv.y));

    outFragColor = vec4(sky.rgb  * (1.0 - clouds.a) + clouds.rgb, 1.0);
}