#pragma once

#include <memory>
#include <vector>
#include <cassert>
#include <queue>

#include "hammock/platform/Window.h"
#include "hammock/core/ResourceManager.h"
#include "hammock/core/SwapChain.h"


namespace Hammock {
    class FrameManager {
    public:
        FrameManager(Window &window, Device &device, ResourceManager &resourceManager );

        ~FrameManager();

        // delete copy constructor and copy destructor
        FrameManager(const FrameManager &) = delete;

        FrameManager &operator=(const FrameManager &) = delete;

        [[nodiscard]] SwapChain *getSwapChain() const { return swapChain.get(); };
        [[nodiscard]] float getAspectRatio() const { return swapChain->extentAspectRatio(); }
        [[nodiscard]] bool isFrameInProgress() const { return isFrameStarted; }


        [[nodiscard]] int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        [[nodiscard]] int getSwapChainImageIndex() const {
            assert(isFrameStarted && "Cannot get image index when frame not in progress");
            return currentImageIndex;
        }

        
        void submitPresentCommandBuffer(VkCommandBuffer commandBuffer, const std::vector<VkSemaphore>& wait, const std::vector<VkPipelineStageFlags>& waitStages);

        bool beginFrame();

        void endFrame();

    private:

        void recreateSwapChain();


        Window &window;
        Device &device;
        ResourceManager &resourceManager;
        std::unique_ptr<SwapChain> swapChain;
        std::vector<ResourceHandle> handlesOfSwapImages;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted{false};
    };
}
