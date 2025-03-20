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
    float phase;
    float ambientStrength;
    int DEBUG_epicView;
    int DEBUG_cheapSampleDistance;
    int DEBUG_maxSamples;
    int DEBUG_maxLightSamples;
    int DEBUG_expensiveSampling;
    int DEBUG_earlyTermination;
    int DEBUG_lateTermination;
    int DEBUG_longStepMulti;
};

[vk::binding(0)] ConstantBuffer<FrameDataUbo> data;

// Storage images
[vk::binding(1)] RWTexture2D cloudsStorageImage;
[vk::binding(2)] RWTexture2D godRaysMaskStorageImage;

// Images and samplers
[vk::binding(3)] Sampler3D lowFreqNoiseTexture;
[vk::binding(4)] Sampler3D highFreqNoiseTexture;
[vk::binding(5)] Sampler2D cloudMapTexture;

// Shadow map
[vk::binding(6)] RWTexture2D<float> shadowMap;

// Push constants
[vk::push_constant] PushConstants constants;

// Camera
#define EYE data.cameraPosition.xyz

// Areal perspective
#define FOG_DISTANCE 70000.0
#define FOG_FACTOR 0.000008

// Sampling
#define NUM_STEPS constants.DEBUG_maxSamples
#define NUM_LIGHT_STEPS constants.DEBUG_maxLightSamples

// Physical properties
#define SCATTERING 1.0
#define ABSORPTION constants.absorption

// Cloud types height density gradients
#define STRATUS_GRADIENT float4(0.0, 0.1, 0.2, 0.3)
#define STRATOCUMULUS_GRADIENT float4(0.02, 0.2, 0.48, 0.625)
#define CUMULUS_GRADIENT float4(0.00, 0.1625, 0.88, 0.98)

// Wind
#define WIND_DIRECTION normalize(data.windDirection.xyz)

// Weather map
#define COVERAGE_REPEAT 9.0

// Earth
#define EARTH_RADIUS 637800
#define CLOUDS_BOTTOM_RADIUS (EARTH_RADIUS + 4000)
#define CLOUDS_TOP_RADIUS (CLOUDS_BOTTOM_RADIUS + 10500)
#define CENTER float3(0.0, -EARTH_RADIUS, 0.0)

// Sun
#define SUN_DIR normalize(data.lightDirection.xyz)
#define SUN_COLOR data.lightColor.rgb
#define SUN_STRENGTH data.lightColor.a

// Noise sampling
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

#define MASK_THRESHOLD 0.01


// Returns a relative hight in cloud layer 0 to 1
float getHeightFraction(float3 inPos) {
    return saturate((length(inPos - CENTER) - CLOUDS_BOTTOM_RADIUS) / (CLOUDS_TOP_RADIUS - CLOUDS_BOTTOM_RADIUS));
}

// Returns gradient for a specific cloud type
float getDensityGradient(float heightFraction, float cloudType){
    float stratusFactor = 1.0 - clamp(cloudType * 2.0, 0.0, 1.0);
    float stratoCumulusFactor = 1.0 - abs(cloudType - 0.5) * 2.0;
    float cumulusFactor = clamp(cloudType - 0.5, 0.0, 1.0) * 2.0;

    float4 baseGradient = stratusFactor * STRATUS_GRADIENT + stratoCumulusFactor * STRATOCUMULUS_GRADIENT + cumulusFactor * CUMULUS_GRADIENT;

    // gradient computation (see Siggraph 2017 Nubis-Decima talk)
    return remap(heightFraction, baseGradient.x, baseGradient.y, 0.0, 1.0) * remap(heightFraction, baseGradient.z, baseGradient.w, 1.0, 0.0);
}

// Spherical uv projection
float2 getUVProjection(float3 p) {
    return p.xz / CLOUDS_BOTTOM_RADIUS + 0.5;
}

// Samples a density at a point
// If expensive, detailed hight-frequency noise will be used to erode the base shape
float sampleCloudDensity(float3 p, bool expensive, float lod) {
    // Init cloud sample
    // Height fraction and uv coords for sampling
    float heightFraction = getHeightFraction(p);
    float3 animation = heightFraction * WIND_DIRECTION + WIND_DIRECTION * data.time * WIND_SPEED;
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

    // Make clouds on the horizon look bigger to create 'epic' view
    if(constants.DEBUG_epicView == 1){
        // Calculate distance from camera
        float distanceFromCamera = length(p - EYE);
        // Define the distance threshold near the max view distance
        float transitiondstToStartance = FOG_DISTANCE * 0.8; // Start at 80% of max distance
        float transitionWidth = FOG_DISTANCE * 0.1; // Transition over 10% of max distance
        // Create a sharp transition that happens only near the horizon
        float distanceFactor = saturate((distanceFromCamera - transitiondstToStartance) / transitionWidth);
        // Use a power function to make the transition even sharper
        distanceFactor = pow(distanceFactor, 4.0);
        // Apply the transition to cloud type
        cloudType = lerp(cloudType, 1.0, distanceFactor);
    }
    

    // Adjust the density based on the cloud type
    float cloudTypeDensity = getDensityGradient(heightFraction, cloudType);
    baseCloud *= (cloudTypeDensity / max(0.001, heightFraction));

    // Apply the coverage
    float baseCloudWithCoverage = remap(baseCloud , coverage, 1.0, 0.0, 1.0);
    baseCloudWithCoverage = saturate(baseCloudWithCoverage * coverage);

   // return saturate(baseCloudWithCoverage);

    // Check if we are doing expensive sampling
    if (expensive){
        // Sample high frequency noise and build the FBM
        float3 highFrequencyNoise = highFreqNoiseTexture.SampleLevel(float3(staticUv * DETAIL_SCALE, heightFraction), lod).rgb * constants.detailMultiplier;
        float highFreqFBM = dot(highFrequencyNoise, float3(0.625, 0.25, 0.125));
        float highFreqNoiseModifier = lerp(highFreqFBM, 1.0 - highFreqFBM, saturate(heightFraction * 10.0));
        baseCloudWithCoverage = remap(baseCloudWithCoverage, highFreqNoiseModifier, 1.0, 0.0, 1.0);
    }

    return saturate(baseCloudWithCoverage);
}

// Compute level of attenuation along a light ray
// Using cone sampling as described by Schneider
float lightRayAttenuation(float3 p) {
    float3 dirToLight = normalize(data.lightDirection.xyz);
    float attenuationAlongLightRay = 1.0;
    float totalDensity = 0.0;
    float stepSize = 0.5 * (CLOUDS_TOP_RADIUS - CLOUDS_BOTTOM_RADIUS) / NUM_LIGHT_STEPS;

    // Cone parameters (as described by Schneider)
    float lastStepDistanceMultiplier = 5.0; // Last step reaches 2x further

    // Lightmarch - exactly NUM_LIGHT_STEPS total samples
    for (int l = 0; l < NUM_LIGHT_STEPS; l++) {
        float t = (float)l / (NUM_LIGHT_STEPS - 1); // Normalized position [0,1]

        // Modify the last step to reach further
        float distanceFactor;
        if (l == NUM_LIGHT_STEPS - 1) {
            distanceFactor = (1.0 + lastStepDistanceMultiplier) / 2.0; // Average of second-to-last and extended last
        } else {
            // Non-uniform step distribution (denser near start, sparser far away)
            distanceFactor = t;
        }

        // Calculate step position - distance increases with each step
        float stepDistance = distanceFactor * NUM_LIGHT_STEPS * stepSize;

        // Calculate cone radius at this distance - widens as we move away
        float coneRadius = stepDistance * tan(0.03); // 0.03 radians = ~1.7 degrees cone angle

        // Offset direction based on step number (spiral pattern)
        float angle = l * 2.4; // Golden angle in radians

        // Create offset vectors perpendicular to light direction
        float3 u, v;
        if (abs(dirToLight.x) < 0.99) {
            u = normalize(cross(float3(1, 0, 0), dirToLight));
        } else {
            u = normalize(cross(float3(0, 1, 0), dirToLight));
        }
        v = cross(dirToLight, u);

        // Calculate radius factor (distance from center line of the cone)
        // t gives distance from center, square root gives uniform disc distribution
        float r = coneRadius * sqrt(t);

        // Calculate final sample position with offset
        float3 offset = r * (cos(angle) * u + sin(angle) * v);
        float3 samplePos = p + dirToLight * stepDistance + offset;

        // Sample cloud density
        float density = sampleCloudDensity(samplePos, false, l * 0.5);

        // Accumulate density
        totalDensity += density * stepSize;
    }

    // Apply Beer's law for exponential light attenuation
    return saturate(max(exp(-totalDensity * ABSORPTION), (exp(-totalDensity * ABSORPTION * 0.25)) * 0.7));
}

// Marches along a ray from the start to the end position
// Uses adaptive stepping: longer steps until cloud is detected, then short steps
float4 raymarch(uint3 threadId, float3 start, float3 end) {
    // Create the ray to march along
    float3 ray = end - start;
    float rayLength = length(ray);
    float3 rayDirection = normalize(ray);

    // Compute the base number of steps to take
    // Cosine of angle between ray and vertical (Y axis)
    float angleFactor = abs(dot(rayDirection, float3(0.0, 1.0, 0.0))); // 1 when looking up, 0 when horizontal
    float numOfSteps = lerp(NUM_STEPS, NUM_STEPS * 0.5, angleFactor);  // fewer steps when looking up
    float baseStepSize = rayLength / numOfSteps;

    // Step size multipliers
    float largeStepMultiplier = float(constants.DEBUG_longStepMulti); // For empty space
    float smallStepMultiplier = 1.0; // For detailed sampling inside clouds

    // Dither the ray start position to combat banding
    int a = int(threadId.x) % 4;
    int b = int(threadId.y) % 4;
    float3 p = start + rayDirection * baseStepSize * bayerFilter[a * 4 + b] * 1.87;

    // Phase function
    float3 L = normalize(SUN_DIR);
    float3 V = normalize(rayDirection);
    float cosTheta = dot(L, V);
    float phase = dualLobePhase(cosTheta, constants.phase);

    // Raymarch variables
    float transmittance = 1.0;
    float accumulatedDensity = 0.0;
    float3 inScatteredLight = 0.0;
    float currentStepSize = baseStepSize * largeStepMultiplier;
    bool insideCloud = false;
    bool everInCloud = false;
    int emptyStepsCount = 0;

    // Maximum distance we can travel
    float remainingDistance = rayLength;

    // Raymarch with adaptive step sizes
    while (remainingDistance > 0.0) {
        // Calculate the distance from the camera
        float distanceFromCamera = length(p - EYE);
        bool expensive = distanceFromCamera < constants.DEBUG_cheapSampleDistance;
        float lod = expensive ? 0.0 : 2.0;

        // Sample the density (use cheap sampling for large steps)
        float density = sampleCloudDensity(p, expensive && insideCloud, lod);

        // Adaptive stepping logic
        if (density > 0.001) {
            // We hit a cloud - if we were taking large steps, we need to back up and switch to small steps
            if (!insideCloud) {
                // Back up one large step and switch to small steps
                if (currentStepSize > baseStepSize) {
                    p -= rayDirection * currentStepSize;
                    remainingDistance += currentStepSize;
                    currentStepSize = baseStepSize * smallStepMultiplier;
                    insideCloud = true;
                    everInCloud = true;
                    emptyStepsCount = 0;
                    continue; // Skip this step and restart with small steps
                } else {
                    // Already using small steps, just mark that we're in a cloud
                    insideCloud = true;
                    everInCloud = true;
                    emptyStepsCount = 0;
                }
            }

            // Process the cloud sample
            accumulatedDensity += density;

            // compute the height fraction: relative height in cloud layer <0;1>
            float heightFraction = getHeightFraction(p);

            // Beer-powder aprox
            float powderTerm = powder(accumulatedDensity);

            // compute the light ray attenuation
            float transmittanceAlongLightRay = lightRayAttenuation(p);

            // in-scattering
            float depthProb = 0.05 * pow(density, remap(heightFraction, 0.3, 0.85, 0.5, 2.0));
            float vertProb = saturate(pow(remap(heightFraction, 0.07, 0.14, 0.1, 1.0), 1.5));
            float inScatterProb = depthProb;//* vertProb;

            // Compute the differential in-scattering contribution using the current transmittance
            inScatteredLight += density * currentStepSize * transmittance * powderTerm * transmittanceAlongLightRay * phase * inScatterProb;

            // Update total transmittance for extinction along the ray segment
            float currentTransmittance = exp(-density * currentStepSize * ABSORPTION);
            transmittance *= currentTransmittance;

            uint2 shadowIndex = clamp(uint2(frac(getUVProjection(p) * COVERAGE_REPEAT) * 512.0), uint2(0, 0), uint2(511, 511));
            shadowMap[shadowIndex] += (1.0 - currentTransmittance);


            // Early exit if fully opaque
            if (transmittance < 0.001) {
                if (constants.DEBUG_earlyTermination == 1) {
                    inScatteredLight = float3(1.0, 0.0, 0.0);
                }
                break;
            }
        } else {

            // No cloud here
            emptyStepsCount++;

            // If we've taken several empty steps while using small steps, switch back to large steps
            if (insideCloud && emptyStepsCount > 3) {
                insideCloud = false;
                currentStepSize = baseStepSize * largeStepMultiplier;
            }
        }

        // Move one step
        p += rayDirection * currentStepSize;
        remainingDistance -= currentStepSize;

        // Safety check to prevent infinite loops
        if (remainingDistance < 0.0 || currentStepSize <= 0.0) {
            break;
        }
    }

    if (constants.DEBUG_lateTermination == 1 && !everInCloud) {
        transmittance = 0.0;
        inScatteredLight =  float3(1.0, 0.0, 0.0);
    }

    // Compute alpha from the accumulated optical depth,
    // Here we base it on the remaining transmittance.
    float alpha = 1.0 - transmittance;
    return float4(inScatteredLight * SUN_COLOR * SUN_STRENGTH, alpha);
}

[shader("compute")]
[numthreads(16,16,1)]
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    int2 resolution = int2(data.resX, data.resY);

    // Compute ray direction.
    float4 rayClip = float4(computeClipSpaceCoord(threadId.xy, resolution), 1.0);
    float4 rayView = mul(data.invProj, rayClip);
    rayView = float4(rayView.xy, -1.0, 0.0);
    float3 rayDirection = mul(data.invView, rayView).xyz;
    rayDirection = normalize(rayDirection);
    float3 rayOrigin = data.cameraPosition.xyz;

    // Compute distance from camera to sphere center.
    float cameraDist = length(rayOrigin - CENTER);
    bool insideInnerSphere = cameraDist < CLOUDS_BOTTOM_RADIUS;
    bool insideOuterSphere = cameraDist < CLOUDS_TOP_RADIUS;
    bool aboveOuterSphere = cameraDist > CLOUDS_TOP_RADIUS;

    // Get intersections.
    float2 planetIntersect = intersectRaySphere(CENTER, EARTH_RADIUS, rayOrigin, rayDirection);
    float2 innerCloudIntersect = intersectRaySphere(CENTER, CLOUDS_BOTTOM_RADIUS, rayOrigin, rayDirection);
    float2 outerCloudIntersect = intersectRaySphere(CENTER, CLOUDS_TOP_RADIUS, rayOrigin, rayDirection);

    // if we intersected a planet, we exit as there are no clouds under ground
    if (planetIntersect.y > 0.0) {
        cloudsStorageImage[threadId.xy] = float4(0.0);
        godRaysMaskStorageImage[threadId.xy / 4] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // We only account for situation where we are between the planet and clouds
    float dstToStart = innerCloudIntersect.y;
    float dstToEnd = outerCloudIntersect.y;


    // Validate segment
    if (dstToEnd <= dstToStart || dstToStart < 0.0) {
        cloudsStorageImage[threadId.xy] = float4(1.0, 0.0, 0.0, 1.0);
        godRaysMaskStorageImage[threadId.xy / 4] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Start and end points
    float3 start = rayOrigin + rayDirection * dstToStart;
    float3 end = rayOrigin + rayDirection * dstToEnd;
    float distanceToPixel = length(end - rayOrigin);

    // Perform raymarching for clouds.
    float4 color = raymarch(threadId,start, end);

    // If the alpha is high enough, write it to the mask, else write the color of the light
    godRaysMaskStorageImage[threadId.xy / 4] = float4(lerp(SUN_COLOR * SUN_STRENGTH, float3(0.0), color.a), 1.0);

    // Store the final value
    cloudsStorageImage[threadId.xy] = color;
}