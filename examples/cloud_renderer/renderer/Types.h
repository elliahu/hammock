#pragma once
#include <hammock/hammock.h>

/**
 * Data that are shader acros all passes
 */
struct GlobalData {
    HmckMat4 invView;
    HmckMat4 invProj;
    HmckMat4 view;
    HmckMat4 proj;
    HmckVec4 cameraPosition;
    HmckVec4 lightColor{1.0f, 1.0f, 1.0f, 5.0f}; // W is strength
    HmckVec4 lightDirection{0.0f, 1.0f, 0.f, 0.0f};
    HmckVec4 skyColorZenith{59.0/255.0, 110.0/255.0, 219.0/255.0};
    HmckVec4 skyColorHorizon{169.0/255.0, 175.0/255.0, 188.0/255.0};
    HmckVec4 windDirection;
    float resX;
    float resY;
    float lowResX;
    float lowResY;
    float fov;
    float time = 0.0f;
    float timeOfDay = 0.45f;
};

/**
 * Contains clouds params values passed as a push constant block
 */
struct CloudsProperties {
    float anvilBias = 0.0f;
    float globalDensity = 0.3f;
    float globalCoverage = 0.0;
    float baseMultiplier = 0.8f;
    float detailMultiplier = 0.75;
    float cloudSpeed = 1000.f;
    float baseScale = 50.f;
    float detailScale = 50.0f;
    float curliness = 2.0f;
    float absorption = 0.0095f;
    float phase = 0.22f;
    float ambientStrength = 0.05;
    int DEBUG_epicView = 1;
    int DEBUG_cheapSampleDistance = 100000;
    int DEBUG_maxSamples = 128;
    int DEBUG_maxLightSamples = 6;
    int DEBUG_expensiveSampling = 0;
    int DEBUG_earlyTermination = 0;
    int DEBUG_lateTermination = 0;
    int DEBUG_longStepMulti = 10;
};

struct BlurProperties {
    HmckVec4 screenSpaceLightPos;
    int numSamples = 128;
    float density = 0.8;
    float exposure;
    float decay = 0.9;
    float weight = 0.7;
    float alpha = 0.85;
    float activeDistance = 1.0f;
};

/**
 * Data that are passed to the terrain shader as a push constant block
 */
struct TerrainData {
    HmckMat4 modelViewProjection;
};

/**
 * Data passed as push block for composition pass
 */
struct CompositionData {

};

/**
 * Data passed as push block for post processing pass
 */
struct PostProcessingData {
    HmckVec4 colorTint{1.0f, 1.0f, 1.0f,0.0f};
    float exposure = 2.3f;        // Default: 0.0, Range: -5.0 to 5.0
    float gamma = 2.2f;           // Default: 2.2, Range: 0.5 to 3.0
    int tonemapOperator = 3;   // 0: Linear, 1: Reinhard, 2: ACES, 3: Uncharted 2
    float contrast = 1.0;        // Default: 1.0, Range: 0.5 to 2.0
    float brightness = 0.0;      // Default: 0.0, Range: -1.0 to 1.0
    float saturation = 1.0;      // Default: 1.0, Range: 0.0 to 2.0
    float vignetteStrength = 2.f; // Default 2.0, Range: 0.0 to 3.0
    float vignetteSoftness = 1.0f; // Default: 0.5, Range: 0.0 to 2.0
    float temperature = 0.0f;     // Default: 0.0, Range: -1.0 (cool) to 1.0 (warm)
    float grainAmount = 0.0f;     // Default: 0.0, Range: 0.0 to 0.1
    float time = 0.0f;
};