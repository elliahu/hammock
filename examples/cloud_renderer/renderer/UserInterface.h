#pragma once
#include <hammock/hammock.h>

namespace hmck = hammock;

class UserInterface final {
    hmck::Device &device;
    hmck::FrameManager &frameManager;
    VkDescriptorPool descriptorPool;
    hmck::Window &window;
    // TODO needs to recreate when swapchain recreates
    hmck::UserInterface ui;

public:
    UserInterface(hmck::Device &device, hmck::FrameManager &frameManager, VkDescriptorPool descriptorPool,
                  hmck::Window &window): device(device), frameManager(frameManager), descriptorPool(descriptorPool), window(window),
                                         ui(device, frameManager.getSwapChain()->getRenderPass(), descriptorPool, window) {
    }

    void recordUserInterface(VkCommandBuffer commandBuffer);

};
