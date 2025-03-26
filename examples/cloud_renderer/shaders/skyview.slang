import toolbox;

[vk::binding(0,0)] ConstantBuffer<AtmosphereParams> params;
[vk::binding(0,1)] Sampler2D Transmittance;
[vk::binding(1,1)] Sampler2D MultipleScattering;
[vk::binding(2,1)] RWTexture2D<float4> SkyView;

#define NUM_SAMPLES 30
#define SUN_INTENSITY float3(10.0)

void getSigmaST(float h, out float3 sigmaS, out float3 sigmaT)
{
    float3 rayleigh = params.ScatterRayleigh.xyz * exp(-h / params.HDensityRayleigh);
    float mieDensity = exp(-h / params.HDensityMie);
    float mieS = params.ScatterMie * mieDensity;
    float mieT = (params.ScatterMie + params.AbsorbMie) * mieDensity;
    float3 ozone = params.AbsorbOzone.xyz * max(0.0f, 1 - 0.5 * abs(h - params.OzoneCenterHeight) / params.OzoneThickness);
    sigmaS = rayleigh + mieS;
    sigmaT = rayleigh + mieT + ozone;
}

float3 evalPhaseFunction(float h, float u)
{
    float3 sRayleigh = params.ScatterRayleigh.xyz * exp(-h / params.HDensityRayleigh);
    float sMie = params.ScatterMie * exp(-h / params.HDensityMie);
    float3 s = sRayleigh + sMie;
    float g = params.AsymmetryMie, g2 = g * g, u2 = u * u;
    float pRayleigh = 3 / (16 * PI) * (1 + u2);
    float m = 1 + g2 - 2 * g * u;
    float pMie = 3 / (8 * PI) * (1 - g2) * (1 + u2) / ((2 + g2) * m * sqrt(m));
    float3 result;
    result.x = s.x > 0 ? (pRayleigh * sRayleigh.x + pMie * sMie) / s.x : 0;
    result.y = s.y > 0 ? (pRayleigh * sRayleigh.y + pMie * sMie) / s.y : 0;
    result.z = s.z > 0 ? (pRayleigh * sRayleigh.z + pMie * sMie) / s.z : 0;
    return result;
}

float3 getTransmittance(Sampler2D T, float h, float theta)
{
    float u = h / (params.AtmosphereRadius - params.PlanetRadius);
    float v = 0.5 + 0.5 * sin(theta);
    return T.SampleLevel(float2(u, v), 0.0).xyz;
}

void marchStep(
    float phaseU, float3 ori, float3 dir, float thisT, float nextT,
    inout float3 sumSigmaT, inout float3 inScattering)
{
    float midT = 0.5 * (thisT + nextT);
    float3 posR = float3(0, ori.y + params.PlanetRadius, 0) + dir * midT;
    float h = length(posR) - params.PlanetRadius;
    
    float3 sigmaS, sigmaT;
    getSigmaST(h, sigmaS, sigmaT);
    
    float3 deltaSumSigmaT = (nextT - thisT) * sigmaT;
    float3 eyeTrans = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);
    
    float sunTheta = acos(dot(-params.SunDirection.xyz, normalize(posR))) - PI / 2;
    
    float3 rho = evalPhaseFunction(h, phaseU);
    float3 sunTrans = getTransmittance(Transmittance, h, sunTheta);
    inScattering += (nextT - thisT) * eyeTrans * sigmaS * rho * sunTrans;
    
    float v_ms = 0.5 + 0.5 * sin(sunTheta);
    float u_ms = h / (params.AtmosphereRadius - params.PlanetRadius);
    float3 ms = MultipleScattering.SampleLevel(float2(u_ms, v_ms), 0).rgb;
    
    inScattering += (nextT - thisT) * eyeTrans * sigmaS * ms;
    
    sumSigmaT += deltaSumSigmaT;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height;
    SkyView.GetDimensions(width, height);
    if(threadIdx.x >= width || threadIdx.y >= height)
        return;
    
    float2 texCoord = float2((threadIdx.x + 0.5) / width, (threadIdx.y + 0.5) / height);
    
    float phi = 2 * PI * texCoord.x;
    float vm = 2 * texCoord.y - 1;
    float theta = sign(vm) * (PI / 2) * vm * vm;
    float sinTheta = sin(theta), cosTheta = cos(theta);
    
    float3 ori = params.Eye.xyz;
    float3 dir = float3(cos(phi) * cosTheta, sinTheta, sin(phi) * cosTheta);
    
    float2 planetOri = float2(0, ori.y + params.PlanetRadius);
    float2 planetDir = float2(cosTheta, sinTheta);
    
    float endT = 0;
    if(!findClosestIntersectionWithCircle(planetOri, planetDir, params.PlanetRadius, endT))
    {
        findClosestIntersectionWithCircle(planetOri, planetDir, params.AtmosphereRadius, endT);
    }
    
    float phaseU = dot(params.SunDirection.xyz, -dir);
    
    float t = 0;
    float3 inScatter = float3(0, 0, 0);
    float3 sumSigmaT = float3(0, 0, 0);
    
    float dt = (endT - t) / NUM_SAMPLES;
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float nextT = t + dt;
        marchStep(phaseU, ori, dir, t, nextT, sumSigmaT, inScatter);
        t = nextT;
    }
    
    SkyView[threadIdx.xy] = float4(inScatter * SUN_INTENSITY, 1.0);
}
