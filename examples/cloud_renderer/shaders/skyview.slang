import toolbox;

[vk::binding(0,0)] ConstantBuffer<AtmosphereParams> params;
[vk::binding(0,1)] Sampler2D Transmittance;
[vk::binding(1,1)] Sampler2D MultipleScattering;
[vk::binding(2,1)] RWTexture2D<float4> SkyView;

#define NUM_SAMPLES 30
#define SUN_INTENSITY float3(8.0)


void marchStep(
    float phaseU, float3 ori, float3 dir, float thisT, float nextT,
    inout float3 sumSigmaT, inout float3 inScattering)
{
    float midT = 0.5 * (thisT + nextT);
    float3 posR = float3(0, ori.y + PlanetRadius, 0) + dir * midT;
    float h = length(posR) - PlanetRadius;
    
    float3 sigmaS, sigmaT;
    getSigmaST(h, sigmaS, sigmaT);
    
    float3 deltaSumSigmaT = (nextT - thisT) * sigmaT;
    float3 eyeTrans = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);
    
    float sunTheta = PI / 2 - acos(dot(params.SunDirection.xyz, normalize(posR)));
    float3 rho = evalPhaseFunction(h, phaseU);
    float3 sunTrans = getTransmittance(Transmittance, h, sunTheta);

    inScattering += (nextT - thisT) * eyeTrans * sigmaS * rho * sunTrans;
 
    float tx = h / (AtmosphereRadius - PlanetRadius);
    float ty = 0.5 + 0.5 * sin(sunTheta);
    float3 ms = MultipleScattering.SampleLevel(float2(tx, ty), 0.0).rgb;
    
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
    
    float2 planetOri = float2(0, ori.y + PlanetRadius);
    float2 planetDir = float2(cosTheta, sinTheta);
    
    float endT = 0;
    if(!findClosestIntersectionWithCircle(planetOri, planetDir, PlanetRadius, endT))
    {
        findClosestIntersectionWithCircle(planetOri, planetDir, AtmosphereRadius, endT);
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
