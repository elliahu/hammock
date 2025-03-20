#pragma once
#include <hammock/hammock.h>

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
    float absorption = 0.006f;
    float phase = 0.22f;
    float ambientStrength = 0.05;
    int DEBUG_epicView = 0;
    int DEBUG_cheapSampleDistance = 100000;
    int DEBUG_maxSamples = 128;
    int DEBUG_maxLightSamples = 6;
    int DEBUG_expensiveSampling = 0;
    int DEBUG_earlyTermination = 0;
    int DEBUG_lateTermination = 0;
    int DEBUG_longStepMulti = 10;
};

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
    float fov;
    float time = 0.0f;
    float timeOfDay = 0.0f;
};

/**
 * Data that are passed to the terrain shader as a push constant block
 */
struct TerrainData {
    HmckMat4 modelViewProjection;
};