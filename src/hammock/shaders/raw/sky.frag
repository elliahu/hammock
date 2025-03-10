#version 450

#include "common/cloudsCommon.glsl"

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 outColor;

//layout (binding = 3) uniform sampler2D skyDomeSampler;

layout (binding = 2) uniform SunAndSkyUBO {
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
    float timeOfDay;
};



void main() {

    vec4 ray_clip = vec4(computeClipSpaceCoord(uvec2(gl_FragCoord.x, gl_FragCoord.y), ivec2(resX,resY)), 1.0);
    vec4 ray_view = inv_proj * ray_clip;
    ray_view = vec4(ray_view.xy, -1.0, 0.0);
    vec3 rayDirection = (inv_view * ray_view).xyz;
    rayDirection = normalize(rayDirection);

    //vec3 skyColor = calculateLight(cameraPosition.xyz, direction, raySphere(sphereCenter, SPHERE_OUTER_RADIUS, cameraPosition.xyz, direction).y) * SUN_COLOR;

    //outColor = vec4(skyColor, 1.0);

    vec3 color = vec3(0.0);
    vec3 sunDirection = normalize(lightDirection.xyz);
    float sun = clamp(dot(sunDirection, rayDirection), 0.0, 1.0 );
    // Base sky color
    color = vec3(0.65,0.65,0.95);
    // Add vertical gradient
    color -= 0.8 * vec3(0.90,0.75,0.90) * rayDirection.y;
    // Add sun color to sky
    color += 1.0 * getSunColor(timeOfDay) * pow(sun, 6.0);
    outColor = vec4(color, 1.0);

    //outColor = vec4(vec3(0.0),1.0);
}