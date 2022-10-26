#version 450

//inputs
layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;

// outputs
layout (location = 0) out vec4 outColor;

struct PointLight
{
    vec4 position;
    vec4 color;
};

layout (set = 0, binding = 0) uniform GlobalUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor; // w is intensity
    vec4 directionalLightDirection; // w is intenisty
    vec4 directionalLightColor; // w ignored
    PointLight pointLights[10];
    int numLights;
} ubo;

// push constants
layout (push_constant) uniform Push
{
    mat4 modelMatrix; // model matrix
    mat4 normalMatrix; // using mat4 bcs alignment requirements
} push;


// spot light
vec3 spotLightPosition = vec3(2.0, -2.0,0.0);
vec3 spotLightDirection = vec3(0.0, 1.0, 0.0); // is aiming down
vec3 spotLightColor = vec3(1);
float spotLightStrength = 0.1;
float spotLightInCut = radians(12.0);
float spotLightOutCut = radians(20.0);


void main()
{
    // pre-calculations shared with all lights
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.inverseView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    // directional light
    vec3 sunDirection = normalize(ubo.directionalLightDirection.xyz);
    float sunDiff = max(dot(surfaceNormal, sunDirection), 0.0);
    vec3 sunDiffuse = sunDiff * (ubo.directionalLightColor.xyz * ubo.directionalLightDirection.w);  


    // point lights
    for(int i = 0; i < ubo.numLights; i++)
    {
        // per-light calculations
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        diffuseLight += intensity * cosAngIncidence;

        //specular component of light
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 512.0);  // higher power -> sharper light
        specularLight += intensity * blinnTerm;
    }

    // spot light
    vec3 lightDir = spotLightPosition - fragPosWorld;
    float theta     = dot(lightDir, normalize(-spotLightDirection));
    float epsilon   = spotLightInCut - spotLightOutCut;
    float intensity = clamp((theta - spotLightOutCut) / epsilon, 0.0, 1.0);  

    vec3 spotDiffuse = intensity * spotLightColor;

	outColor = vec4((
        (diffuseLight * fragColor) + 
        (specularLight * fragColor) +
        (sunDiffuse * fragColor) +
        (spotDiffuse * fragColor)
    ), 1.0);
}