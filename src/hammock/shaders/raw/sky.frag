#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 outColor;

//layout (binding = 3) uniform sampler2D skyDomeSampler;

layout (binding = 2) uniform SunAndSkyUBO {
    vec4 cloudColorTop;
    vec4 cloudColorBottom;
    vec4 lightColor;
    vec4 lightDirection;
    vec4 skyColorBottom;
    vec4 skyColorTop;
};

layout (binding = 0) uniform CameraUBO
{
    mat4 inv_view;
    mat4 inv_proj;
    mat4 invViewProj;
    vec4 cameraPosition;
    float resX;
    float resY;
    float fov;
};

layout (binding = 1) uniform TimeUBO
{
    float iTime;
};


const float PI = 3.14159265359;

void main(){
    vec4 direction_view = invViewProj * vec4(in_uv * 2.0 - 1.0, 1.0, 1.0);
    vec3 direction = normalize(direction_view.xyz / direction_view.w);

    // Convert 3D direction to 2D UV coordinates using equirectangular projection
    vec2 uv;
    uv.x = 0.5 + 0.5 * atan(direction.z, direction.x) / PI;
    uv.y = 0.5 - asin(clamp(direction.y, -1.0, 1.0)) / PI;


    // Adjust gradient to have skyColorBottom at the horizon (y = 0) and skyColorTop at the top (y = 1)
    float gradientFactor = clamp(direction.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 skyColor = mix(skyColorBottom.rgb, skyColorTop.rgb, gradientFactor);

    outColor = vec4(skyColor, 1.0);

}