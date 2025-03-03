#version 450

layout (binding = 0) uniform sampler2D colorSampler;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = texture(colorSampler, vec2(uv.x, 1.0 - uv.y));
}