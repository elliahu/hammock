import toolbox;

#define WORLD_SCALE 200.0
#define MAX_DISTANCE 60000
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


float relativeLuminance(float3 c)
{
    return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
}

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

    float AtmosEyeHeight = params.Eye.y * WORLD_SCALE;

    float3 ori = float3(0, AtmosEyeHeight, 0);
    float3 dir = normalize(lerp(
        lerp(data.FrustumA, data.FrustumB, xf), lerp(data.FrustumC, data.FrustumD, xf), yf));

    float u = dot(params.SunDirection.xyz, -dir);

    float maxT = 0;
    if(!findClosestIntersectionWithSphere(
        ori + float3(0, params.PlanetRadius, 0), dir, params.PlanetRadius, maxT))
    {
        findClosestIntersectionWithSphere(
            ori + float3(0, params.PlanetRadius, 0), dir, params.AtmosphereRadius, maxT);
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
            float3 posR = float3(0, ori.y + params.PlanetRadius, 0) + dir * midT;
            float  h    = length(posR) -params.PlanetRadius;

            float SunTheta = asin(-params.SunDirection.y);

            float3 sigmaS, sigmaT;
            getSigmaST(h, sigmaS, sigmaT);

            float3 deltaSumSigmaT = dt * sigmaT;
            float3 eyeTrans = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

            if(!hasIntersectionWithSphere(posR, -params.SunDirection.xyz, params.PlanetRadius))
            {
                float3 shadowPos  = params.Eye.xyz + dir * midT / WORLD_SCALE;
                float4 shadowClip = mul(float4(shadowPos, 1), data.ShadowViewProj);
                float2 shadowNDC  = shadowClip.xy / shadowClip.w;
                float2 shadowUV   = 0.5 + float2(0.5, -0.5) * shadowNDC;

                bool inShadow = true;
                if(all(saturate(shadowUV) == shadowUV))
                {
                    float rayZ = shadowClip.z;
                    float smZ = ShadowMap.SampleLevel(shadowUV, 0).r;
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

                float tx = h / (params.AtmosphereRadius - params.PlanetRadius);
                float ty = 0.5 + 0.5 * sin(SunTheta);
                float3 ms = MultipleScattering.SampleLevel( float2(tx, ty), 0).rgb;
                inScatter += dt * eyeTrans * sigmaS * ms;
            

            sumSigmaT += deltaSumSigmaT;
            t = nextT;
        }

        float transmittance = relativeLuminance(exp(-sumSigmaT));
        AerialPerspective[int3(threadIdx.xy, z)] =
            float4(inScatter, transmittance);

        tBeg = tEnd;
        tEnd = min(tEnd + sliceDepth, maxT);
    }
}