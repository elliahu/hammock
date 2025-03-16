import toolbox;

struct CloudSample {
    float density = 0.0;
    float coverage = 0.0;
    float type = 0.0;
};


struct PushConstants {
    float anvilBias;
    float globalDensity;
    float globalCoverage;
    float baseMultiplier;
    float detailMultiplier;
    float cloudSpeed;
    float baseScale;
    float detailScale;
    float curliness;
    float absorption;
    float scattering;
    float eccentricity;
    float silverIntensity;
    float silverSpread;
    float ambientStrength;
    int DEBUG_cheapSampleDistance;
    int DEBUG_maxSamples;
    int DEBUG_maxLightSamples;
    int DEBUG_expensiveSampling;
    int DEBUG_earlyTermination;
    int DEBUG_lateTermination;
};


[vk::binding(0)] ConstantBuffer<CameraBuffer> cameraBuffer;
[vk::binding(1)] ConstantBuffer<TimeBuffer> timeBuffer;
[vk::binding(2)] ConstantBuffer<SunAndSkyBuffer> sunAndSkyBuffer ;

// Storage image
[vk::binding(3)] RWTexture2D<float4> storageImage;

// Images and samplers
[vk::binding(4)] Sampler3D lowFreqNoiseTexture;
[vk::binding(5)] Sampler3D highFreqNoiseTexture;
[vk::binding(6)] Sampler2D curlNoiseTexture;
[vk::binding(7)] Sampler2D cloudMapTexture;
[vk::binding(8)] Sampler2D skyTexture;

[vk::push_constant] PushConstants constants;

#define EYE cameraBuffer.cameraPosition.xyz

#define NUM_STEPS constants.DEBUG_maxSamples

#define NUM_LIGHT_STEPS constants.DEBUG_maxLightSamples
#define LIGHT_STEP_SIZE 400.0


#define SCATTERING constants.scattering
#define ABSORPTION constants.absorption

// Cloud types height density gradients
#define STRATUS_GRADIENT float4(0.0, 0.1, 0.2, 0.3)
#define STRATOCUMULUS_GRADIENT float4(0.02, 0.2, 0.48, 0.625)
#define CUMULUS_GRADIENT float4(0.00, 0.1625, 0.88, 0.98)

// Wind
#define WIND_DIRECTION normalize(sunAndSkyBuffer.windDirection.xyz)

#define COVERAGE_REPEAT 9.0

// Earth
#define EARTH_RADIUS 637800
#define CLOUDS_BOTTOM_RADIUS (EARTH_RADIUS + 4000)
#define CLOUDS_TOP_RADIUS (CLOUDS_BOTTOM_RADIUS + 9000)
#define CENTER float3(0.0, -EARTH_RADIUS, 0.0)

// Sun
#define SUN_DIR normalize(sunAndSkyBuffer.lightDirection.xyz)
#define SUN_COLOR sunAndSkyBuffer.lightColor.rgb * 1.5

// Cloud sampling
#define BASE_SCALE constants.baseScale
#define DETAIL_SCALE constants.detailScale
#define WIND_SPEED constants.cloudSpeed

// Bayer filter
#define BAYER_FACTOR 1.0 / 16.0
static const float bayerFilter[16u] = float[]
(
0.0 * BAYER_FACTOR, 8.0 * BAYER_FACTOR, 2.0 * BAYER_FACTOR, 10.0 * BAYER_FACTOR,
12.0 * BAYER_FACTOR, 4.0 * BAYER_FACTOR, 14.0 * BAYER_FACTOR, 6.0 * BAYER_FACTOR,
3.0 * BAYER_FACTOR, 11.0 * BAYER_FACTOR, 1.0 * BAYER_FACTOR, 9.0 * BAYER_FACTOR,
15.0 * BAYER_FACTOR, 7.0 * BAYER_FACTOR, 13.0 * BAYER_FACTOR, 5.0 * BAYER_FACTOR
);


static const float3 noiseKernel[6u] = {
    float3(0.38051305, 0.92453449, -0.02111345),
    float3(-0.50625799, -0.03590792, -0.86163418),
    float3(-0.32509218, -0.94557439, 0.01428793),
    float3(0.09026238, -0.27376545, 0.95755165),
    float3(0.28128598, 0.42443639, -0.86065785),
    float3(-0.16852403, 0.14748697, 0.97460106)
};


// Returns a relative hight in cloud layer 0 to 1
float getHeightFraction(float3 inPos) {
    return (length(inPos - CENTER) - CLOUDS_BOTTOM_RADIUS) / (CLOUDS_TOP_RADIUS - CLOUDS_BOTTOM_RADIUS);
}

// Returns density for a specific cloud type
// 0.0 stratus
// 0.5 stratocumulus
// 1.0 cumulus
float getDensityForCloudType(float heightFraction, float cloudType){
    float stratusFactor = 1.0 - clamp(cloudType * 2.0, 0.0, 1.0);
    float stratoCumulusFactor = 1.0 - abs(cloudType - 0.5) * 2.0;
    float cumulusFactor = clamp(cloudType - 0.5, 0.0, 1.0) * 2.0;

    float4 baseGradient = stratusFactor * STRATUS_GRADIENT + stratoCumulusFactor * STRATOCUMULUS_GRADIENT + cumulusFactor * CUMULUS_GRADIENT;

    // gradicent computation (see Siggraph 2017 Nubis-Decima talk)
    return remap(heightFraction, baseGradient.x, baseGradient.y, 0.0, 1.0) * remap(heightFraction, baseGradient.z, baseGradient.w, 1.0, 0.0);
}

// spherical uv projection
float2 getUVProjection(float3 p) {
    return p.xz / CLOUDS_BOTTOM_RADIUS + 0.5;
}

// Smples a density at a point
// If expensive, detail hight-frequency noise will be used to erode the base shape
float sampleCloudDensity(float3 p, bool expensive, float lod) {
    // Init cloud sample
    // Height fraction and uv coords for sampling
    float heightFraction = getHeightFraction(p);
    float3 animation = heightFraction * WIND_DIRECTION + WIND_DIRECTION * timeBuffer.time * WIND_SPEED;
    float2 staticUv = getUVProjection(p);
    float2 animatedUv = getUVProjection(p + animation);

    // Early exit if out of the cloud layer
    if (heightFraction < 0.0 || heightFraction > 1.0) {
        return 0.0;
    }


    // Sample low frequency noise to construct FBM
    float4 lowFrequencyNoise = lowFreqNoiseTexture.SampleLevel(float3(animatedUv * BASE_SCALE, heightFraction), lod);
    float lowFreqFBM = dot(lowFrequencyNoise.gba, float3(0.625, 0.25, 0.125));
    float baseCloud = remap(lowFrequencyNoise.r, lowFreqFBM - 1.0, 1.0, 0.0, 1.0) * constants.globalDensity;

    // Sample the cloud map
    float3 cloudMap = cloudMapTexture.SampleLevel(animatedUv * COVERAGE_REPEAT, 0.0).rgb * constants.baseMultiplier;
    // Coverage is in red and green channels
    float coverage = max(cloudMap.r, saturate(constants.globalCoverage - 0.5) * cloudMap.g * 2);
    coverage = pow(coverage, remap(heightFraction, 0.7, 0.8, 1.0, lerp(1.0, 0.5, constants.anvilBias)));
    // Cloud type is in the blue channel
    float cloudType = cloudMap.b;


    // Adjust the density based on the cloud type
    float cloudTypeDensity = getDensityForCloudType(heightFraction, cloudType);
    baseCloud *= (cloudTypeDensity / max(0.001, heightFraction));

    // Apply the coverage
    float baseCloudWithCoverage = remap(baseCloud , coverage, 1.0, 0.0, 1.0);
    baseCloudWithCoverage = saturate(baseCloudWithCoverage * coverage);


    // Check if we are doing expensive sampling
    if (expensive || (constants.DEBUG_expensiveSampling == 1))
    {
        // Sample hight frequency noise and build the FBM
        float3 highFrequencyNoise = highFreqNoiseTexture.SampleLevel(float3(staticUv * DETAIL_SCALE, heightFraction), lod).rgb * constants.detailMultiplier;
        float highFreqFBM = dot(highFrequencyNoise, float3(0.625, 0.25, 0.125));

        float highFreqNoiseModifier = lerp(highFreqFBM, 1.0 - highFreqFBM, saturate(heightFraction * 10.0));
        baseCloudWithCoverage = remap(baseCloudWithCoverage, highFreqNoiseModifier, 1.0, 0.0, 1.0);
    }

    return saturate(baseCloudWithCoverage);
}


// Compute level of attenuation along a light ray
// TODO: cone sampling
float lightRayAttenuation(float3 p) {
    float3 dirToLight = normalize(sunAndSkyBuffer.lightDirection.xyz);
    float attenuationAlongLightRay = 1.0;
    float accumulatedDensity = 0.0;

    float dstOut = intersectRaySphere(CENTER, CLOUDS_TOP_RADIUS, p, dirToLight).y;
    float stepSize = LIGHT_STEP_SIZE; // dstOut / NUM_LIGHT_STEPS;

    // Lightmarch
    for (int l = 0; l < NUM_LIGHT_STEPS; l++) {
        float3 pos = p + dirToLight * l * stepSize;
        float heightFraction = getHeightFraction(pos);

        if (heightFraction >= 0) {
            float cloudType = 0.0;
            float cloudDensity = sampleCloudDensity(pos, false, l / 16);
            accumulatedDensity += cloudDensity * stepSize;
            if (cloudDensity > 0.0) {
                // Beers law
                float attenuation = exp(-accumulatedDensity * ABSORPTION);
                attenuationAlongLightRay *= attenuation;
            }
        }

        if (attenuationAlongLightRay < 0.01) break;
    }

    return saturate(attenuationAlongLightRay);
}

// Raymarches from the start to the end position
float4 raymarch(uint3 threadId, float3 start, float3 end, float3 skyColor) {
    // Create the ray to march along
    float3 ray = end - start;
    float rayLenght = length(ray);
    float3 rayDirection = normalize(ray);

    // Cosine of angle between ray and vertical (Y axis)
    float angleFactor = abs(dot(rayDirection, float3(0.0, 1.0, 0.0))); // 1 when looking up, 0 when horizontal
    float numOfSteps = lerp(NUM_STEPS, NUM_STEPS * 0.5, angleFactor); // fewer steps when looking up
    float stepSize = rayLenght / numOfSteps;

    // dither the ray start position to combat banding
    uint2 fragCoord = threadId.xy;
    int a = int(fragCoord.x) % 4;
    int b = int(fragCoord.y) % 4;
    float3 p = start + rayDirection * stepSize * bayerFilter[a * 4 + b];

    // Phase function
    float3 L = normalize(SUN_DIR);
    float3 V = normalize(rayDirection);
    float cosTheta = dot(L, V);
    float phase = directedPhase(cosTheta, constants.eccentricity, constants.silverIntensity, constants.silverSpread);

    // Raymarch variables
    float totalAttenuation = 1.0;
    float totalDensity= 0.0;
    float3 inScatteredLight = 0.0;

    // Raymarch
    // TODO optimization: march with big steps until dense region, then backstep one step and sample small steps. if multiple small steps return 0 density, do large steps again
    // TODO at distance do only cheap sampling
    for (int i = 0; i < numOfSteps; i++) {

        // Calculate the distance from the camera
        float distanceFromCamera = length(p - EYE);
        bool expensive = distanceFromCamera < constants.DEBUG_cheapSampleDistance;

        // Sample the density
        float density = sampleCloudDensity(p, expensive, i / 16);
        totalDensity += density * stepSize;

        // Expensive computation only for non-zero density regions
        if (density > 0.01) {
            // Update total transmittance for extinction along the ray segment
            float attenuation = min(.95, exp(-totalDensity  * ABSORPTION));
            totalAttenuation *= attenuation;

            // compute the height fraction: relative height in cloud layer <0;1>
            float heightFraction = getHeightFraction(p);

            // Beer-powder aprox
            float powderTerm = powder(totalDensity);

            // extra in scattering
            float extra = constants.eccentricity * pow(saturate(cosTheta), constants.silverSpread);
            phase = max(phase, extra);

            // compute the light ray attenuation
            float attenuationAlongLightRay = lightRayAttenuation(p);

            // Sunlight
            float3 sunLight = SUN_COLOR * 1.0 * attenuationAlongLightRay;

            // Ambient light
            float3 ambient = skyColor * (1.0 - saturate(constants.ambientStrength * pow(density,remap(heightFraction, 0.3,0.9,0.5,1.0))) * saturate(pow(remap(heightFraction,0.0,0.3,0.8,1.0), 0.8)));

            float3 lightContribution = sunLight + ambient * heightFraction;

            // Compute the differential in-scattering contribution using the current transmittance
            float3 dL = totalAttenuation * density * SCATTERING* phase * lightContribution * powderTerm * stepSize;
            inScatteredLight += dL;  
        }

        // Early exit if fully opaque
        if (totalAttenuation < 0.01) {
            if(constants.DEBUG_earlyTermination == 1){
                inScatteredLight = float3(1.0,0.0,0.0);
            }
            break;
        }

        // Move one step
        p += rayDirection * stepSize;
    }

    // Compute alpha from the accumulated optical depth,
    // Here we base it on the remaining transmittance.
    float alpha = 1.0 - totalAttenuation;
    return float4(inScatteredLight, alpha);
}

[shader("compute")]
[numthreads(16,16,1)]
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint2 fragCoord = uint2(threadId.xy);
    int2 resolution = int2(cameraBuffer.resX, cameraBuffer.resY);

    // Compute ray direction.
    float4 rayClip = float4(computeClipSpaceCoord(fragCoord, resolution), 1.0);
    float4 rayView = mul(cameraBuffer.inv_proj, rayClip);
    rayView = float4(rayView.xy, -1.0, 0.0);
    float3 rayDirection = mul(cameraBuffer.inv_view, rayView).xyz;
    rayDirection = normalize(rayDirection);
    float3 rayOrigin = cameraBuffer.cameraPosition.xyz;

    float4 v = float4(0.0);
    float2 normCoord = float2(fragCoord) / float2(resolution);
    float4 skyColor = skyTexture.SampleLevel(normCoord, 0.0);

    // Compute distance from camera to sphere center.
    float cameraDist = length(rayOrigin - CENTER);
    bool insideInnerSphere = cameraDist < CLOUDS_BOTTOM_RADIUS;
    bool insideOuterSphere = cameraDist < CLOUDS_TOP_RADIUS;
    bool aboveOuterSphere = cameraDist > CLOUDS_TOP_RADIUS;

    // Get intersections.
    float2 planetIntersect = intersectRaySphere(CENTER, EARTH_RADIUS, rayOrigin, rayDirection);
    float2 innerCloudIntersect = intersectRaySphere(CENTER, CLOUDS_BOTTOM_RADIUS, rayOrigin, rayDirection);
    float2 outerCloudIntersect = intersectRaySphere(CENTER, CLOUDS_TOP_RADIUS, rayOrigin, rayDirection);
    float outerCloudExit = outerCloudIntersect.x + outerCloudIntersect.y; // actual exit distance

    float startDist, endDist;

    // Case 1: Camera is inside inner cloud sphere.
    if (insideInnerSphere) {
        startDist = 0.0; // Start at camera.
        if (planetIntersect.x > 0.0 && planetIntersect.x < outerCloudExit) {
            endDist = planetIntersect.x; // Stop if planet is hit.
        } else {
            endDist = outerCloudExit;
        }
    }
    // Case 2: Camera is between inner and outer cloud spheres.
    else if (insideOuterSphere) {
        startDist = max(0.0, innerCloudIntersect.x); // Start at camera or inner sphere entry.
        if (planetIntersect.x > 0.0 && planetIntersect.x < outerCloudExit) {
            endDist = planetIntersect.x; // Clip clouds by the planet.
        } else {
            endDist = outerCloudExit;
        }
    }
    // Case 3: Camera is above the outer cloud sphere.
    else if (aboveOuterSphere) {
        // Allow clouds even if the entry is exactly 0.
        if (outerCloudIntersect.x < 0.0) {
            storageImage[threadId.xy] = float4(0.0);
            return;
        }
        startDist = outerCloudIntersect.x; // Enter at outer sphere.
        if (innerCloudIntersect.x > 0.0) {
            // If ray goes through clouds and then enters the inner sphere...
            if (planetIntersect.x > 0.0 && planetIntersect.x < innerCloudIntersect.x) {
                endDist = planetIntersect.x;
            } else {
                endDist = innerCloudIntersect.x;
            }
        } else {
            // Ray enters outer sphere but doesn't hit inner cloud sphere.
            if (planetIntersect.x > 0.0 && planetIntersect.x < outerCloudExit) {
                endDist = planetIntersect.x;
            } else {
                endDist = outerCloudExit;
            }
        }
    }

    // Validate segment.
    if (endDist <= startDist || startDist < 0.0) {
        storageImage[threadId.xy] = float4(0.0);
        return;
    }

    float3 start = rayOrigin + rayDirection * startDist;
    float3 end = rayOrigin + rayDirection * endDist;

    // Perform raymarching for clouds.
    float4 color = raymarch(threadId,start, end, skyColor.rgb);

    // Apply areal perspective
    color.a *= exp(-length(start - rayOrigin) * .00001);

    storageImage[threadId.xy] = color;
}