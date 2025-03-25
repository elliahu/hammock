import toolbox;

#define STEP_COUNT 1000

[vk::binding(0,0)] ConstantBuffer<AtmosphereParams> params;
[vk::binding(0,1)] RWTexture2D<float4> Transmittance;


float3 getSigmaT(float h){
    float3 rayleigh = params.ScatterRayleigh.xyz * exp(-h / params.HDensityRayleigh);
    float mie = (params.ScatterMie + params.AbsorbMie) * exp(-h / params.HDensityMie);
    float3 ozone = params.AbsorbOzone.xyz * max(
        0.0f, 1 - 0.5 * abs(h - params.OzoneCenterHeight) / params.OzoneThickness);
    return rayleigh + mie + ozone;
}


[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(int3 threadIdx : SV_DispatchThreadID){
    int width, height;
    Transmittance.GetDimensions(width, height);
    if(threadIdx.x >= width || threadIdx.y >= height)
        return;

    float theta = asin(lerp(-1.0, 1.0, float(threadIdx.y) / float(height)));
    float h = lerp(0.0, params.AtmosphereRadius - params.PlanetRadius, float(threadIdx.x) / float(width));

    float2 o = float2(0, params.PlanetRadius + h);
    float2 d = float2(cos(theta), sin(theta));

    float t = 0;
    if(!findClosestIntersectionWithCircle(o, d, params.PlanetRadius, t))
        findClosestIntersectionWithCircle(o, d, params.AtmosphereRadius, t);

    float2 end = o + t * d;

    float3 sum = 0.;
    for(int i = 0; i < STEP_COUNT; ++i)
    {
        float2 pi = lerp(o, end, float(i) / float(STEP_COUNT));
        float hi = length(pi) - params.PlanetRadius;
        float3 sigma = getSigmaT(hi);
        sum += sigma;
    }

    float3 result = exp(-sum * (t / float(STEP_COUNT)));
    Transmittance[threadIdx.xy] = float4(result, 1);
}