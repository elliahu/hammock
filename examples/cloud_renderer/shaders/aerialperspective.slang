import toolbox;

#define MAX_DISTANCE 60000.0
#define PER_SLICE_SAMPLES 1


[vk::binding(0,0)] ConstantBuffer<AtmosphereParams> params;
[vk::binding(0,1)] Sampler2D Transmittance;
[vk::binding(1,1)] Sampler2D MultipleScattering;
[vk::binding(2,1)] Sampler2D ShadowMap;
[vk::binding(3,1)] RWTexture3D<float4> AerialPerspective;

struct Data{
    float4x4 ShadowViewProj;
    float3 FrustumA;
    float3 FrustumB;
    float3 FrustumC;
    float3 FrustumD;
}

[vk_push_constant] Data data;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height, depth;
    AerialPerspective.GetDimensions(width, height, depth);
    if(threadIdx.x >= width || threadIdx.y >= height)
        return;

    float xf = (threadIdx.x + 0.5) / width;
    float yf = (threadIdx.y + 0.5) / height;

    float AtmosEyeHeight = params.Eye.y;

    float3 ori = float3(0, AtmosEyeHeight, 0);
    float3 dir = normalize(lerp(
        lerp(data.FrustumA, data.FrustumB, xf), lerp(data.FrustumC, data.FrustumD, xf), yf));

    float u = dot(params.SunDirection.xyz, dir);

    float maxT = 0;
    if(!findClosestIntersectionWithSphere(
        ori + float3(0, PlanetRadius, 0), dir, PlanetRadius, maxT))
    {
        findClosestIntersectionWithSphere(
            ori + float3(0, PlanetRadius, 0), dir, AtmosphereRadius, maxT);
    }

    float sliceDepth = MAX_DISTANCE / depth;
    float halfSliceDepth = 0.5 * sliceDepth;
    float tBeg = 0, tEnd = min(halfSliceDepth, maxT);

    float3 sumSigmaT = float3(0, 0, 0);
    float3 inScatter = float3(0, 0, 0);

    float rand = frac(sin(dot(
        float2(xf, yf), float2(12.9898, 78.233) * 2.0)) * 43758.5453);

    for(int z = 0; z < depth; ++z)
    {
        float dt = (tEnd - tBeg) / PER_SLICE_SAMPLES;
        float t = tBeg;

        for(int i = 0; i < PER_SLICE_SAMPLES; ++i)
        {
            float nextT = t + dt;

            float  midT = lerp(t, nextT, rand);
            float3 posR = float3(0, ori.y + PlanetRadius, 0) + dir * midT;
            float  h    = length(posR) -PlanetRadius;

            float SunTheta = PI/2 - acos(dot(params.SunDirection.xyz, normalize(posR)));

            float3 sigmaS, sigmaT;
            getSigmaST(h, sigmaS, sigmaT);

            float3 deltaSumSigmaT = dt * sigmaT;
            float3 eyeTrans = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

            if(!hasIntersectionWithSphere(posR, params.SunDirection.xyz, PlanetRadius))
            {
                float3 shadowPos  = params.Eye.xyz + dir * midT;
                float4 shadowClip = mul(data.ShadowViewProj, float4(shadowPos, 1.0));
                float3 shadowNDC  = shadowClip.xyz / shadowClip.w;
                float2 shadowUV = shadowNDC.xy * 0.5 + 0.5;

                bool inShadow = true;
                if(all(saturate(shadowUV) == shadowUV))
                {
                    float rayZ = shadowClip.z;
                    float smZ = ShadowMap.SampleLevel(float2(shadowUV.x, 1.0 - shadowUV.y), 0).r;
                    inShadow = rayZ >= smZ;
                }

                if(!inShadow)
                {
                    float3 rho = evalPhaseFunction(h, u);
                    float3 sunTrans = getTransmittance(
                        Transmittance, h, SunTheta);
                    inScatter += dt * eyeTrans * sigmaS * rho * sunTrans;
                }
            }

                float tx = h / (AtmosphereRadius - PlanetRadius);
                float ty = 0.5 + 0.5 * sin(SunTheta);
                float3 ms = MultipleScattering.SampleLevel( float2(tx, ty), 0).rgb;
                inScatter += dt * eyeTrans * sigmaS * ms;
            

            sumSigmaT += deltaSumSigmaT;
            t = nextT;
        }

        float transmittance = relativeLuminance(exp(-sumSigmaT));
        AerialPerspective[int3(threadIdx.xy, z)] = float4(inScatter, transmittance);
       // AerialPerspective[int3(threadIdx.xy, z)] = float4(dir, transmittance);

        tBeg = tEnd;
        tEnd = min(tEnd + sliceDepth, maxT);
    }
}