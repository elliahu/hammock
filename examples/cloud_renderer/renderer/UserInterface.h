#pragma once
#include <hammock/hammock.h>
#include "Types.h"
#include "Camera.h"

namespace hmck = hammock;

class UserInterface final {
    hmck::Device &device;
    hmck::FrameManager &frameManager;
    VkDescriptorPool descriptorPool;
    hmck::Window &window;
    hmck::UserInterface ui;

    struct CameraRef {
        float *position;
        float *yaw;
        float *pitch;
        float *roll;
        float *fov;
        float *zfar;
    } cameraRef;

    struct GlobalRef {
        float *lightColor;
        float *lightDirection;
        float *skyColorZenith;
        float *skyColorHorizon;
        float *sunLightStrength;
        float *timeOfDay;
    } globalRef;

    struct PostProcessRef {
        float *tint;
        float *exposure;
        float *gamma;
        int *tonemapOperator;
        float *contrast;
        float *brightness;
        float *saturation;
        float *vignetteStrength;
        float *vignetteSoftness;
        float *temperature;
        float *grainAmount;
    } postRef;

    struct CloudsRef {
        float *anvilBias;
        float *globalDensity;
        float *globalCoverage;
        float *baseMultiplier;
        float *detailMultiplier;
        float *cloudSpeed;
        float *baseScale;
        float *detailScale;
        float *curliness;
        float *absorption;
        float *eccentricity;
        float *intensity;
        float *spread;
        float *ambientStrength;
        int *DEBUG_epicView;
        int *DEBUG_cheapSampleDistance;
        int *DEBUG_maxSamples;
        int *DEBUG_maxLightSamples;
        int *DEBUG_expensiveSampling;
        int *DEBUG_earlyTermination;
        int *DEBUG_lateTermination;
        int *DEBUG_longStepMulti;
    } cloudsRef;


    float &deltaTime;
    float *frameTimes;
    int FRAMETIME_BUFFER_SIZE;
    int &frameTimeFrameIndex;

    bool showEditor = true;
    bool showDebug = true;
    bool showPostProc = false;
    bool showCamera = false;
    bool hideAll = false;

    void showCameraWindow();

    void showPostProcsSettingsWindow();

    void showDebugWindow();

    void showEditorWindow();

public:
    UserInterface(
        hmck::Device &device,
        hmck::FrameManager &frameManager,
        VkDescriptorPool descriptorPool,
        hmck::Window &window,
        float &deltaTime,
        float *frameTimes,
        int FRAMETIME_BUFFER_SIZE,
        int &frameTimeFrameIndex
    ): device(device),
       frameManager(frameManager),
       descriptorPool(descriptorPool),
       window(window),
       ui(device, frameManager.getSwapChain()->getRenderPass(), descriptorPool, window),
       deltaTime(deltaTime),
       frameTimes(frameTimes),
       FRAMETIME_BUFFER_SIZE(FRAMETIME_BUFFER_SIZE),
       frameTimeFrameIndex(frameTimeFrameIndex) {
    }

    void setCameraRef(CameraRef ref) { cameraRef = ref; }
    void setGlobalRef(GlobalRef ref) { globalRef = ref; }
    void setPostProcRef(PostProcessRef pref) { postRef = pref; }
    void setCloudsRef(CloudsRef ref) { cloudsRef = ref; }

    void recordUserInterface(VkCommandBuffer commandBuffer);
};
