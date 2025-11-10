#pragma once

#include <cassert>
#include <memory>
#include <vector>


#include "hammock/core/SwapChain.h"
#include "hammock/platform/Window.h"
#include "hammock/utils/Singleton.h"

namespace Hammock {
    class FrameManager: public Singleton<FrameManager> {
        friend class Singleton<FrameManager>;

       public:
        ~FrameManager();

        static void initialize(SurfaceProvider& i_surfaceProvider, Device& device) {
            Singleton<FrameManager>::initialize(i_surfaceProvider, device);
        }

        // delete copy constructor and copy destructor
        FrameManager(const FrameManager&) = delete;
        FrameManager& operator=(const FrameManager&) = delete;

        [[nodiscard]] SwapChain* getSwapChain() const { return swapChain.get(); };
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

        void submitPresentCommandBuffer(VkCommandBuffer commandBuffer, const std::vector<VkSemaphore>& wait,
            const std::vector<VkPipelineStageFlags>& waitStages);

        bool beginFrame();

        void endFrame();

       protected:
        FrameManager(SurfaceProvider& i_surfaceProvider, Device& device);

        void recreateSwapChain();

        SurfaceProvider& surfaceProvider;
        Device& device;
        std::unique_ptr<SwapChain> swapChain;
        std::vector<ResourceHandle> handlesOfSwapImages;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted{false};
    };
}  // namespace Hammock
