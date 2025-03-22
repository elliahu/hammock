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
    // TODO needs to recreate when swapchain recreates
    hmck::UserInterface ui;

    Camera& camera;
    GlobalData& globalData;
    PostProcessingData& postProcessingData;

    float& deltaTime;
    float* frameTimes;
    int FRAMETIME_BUFFER_SIZE;
    int& frameTimeFrameIndex;

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
        Camera& camera,
        GlobalData& globalData,
        PostProcessingData& postProcessingData,
        float& deltaTime,
        float* frameTimes,
        int FRAMETIME_BUFFER_SIZE,
        int& frameTimeFrameIndex
        ):
    device(device),
    frameManager(frameManager),
    descriptorPool(descriptorPool),
    window(window),
    ui(device, frameManager.getSwapChain()->getRenderPass(), descriptorPool, window),
    camera(camera),
    globalData(globalData),
    postProcessingData(postProcessingData),
    deltaTime(deltaTime),
    frameTimes(frameTimes),
    FRAMETIME_BUFFER_SIZE(FRAMETIME_BUFFER_SIZE),
    frameTimeFrameIndex(frameTimeFrameIndex)
    {
    }

    void recordUserInterface(VkCommandBuffer commandBuffer);

};
