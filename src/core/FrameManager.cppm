module;
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>
#include <memory>

export module hammock.core.frame_manager;

import hammock.core.device;
import hammock.core.utilities;
import hammock.core.base_surface_provider;
import hammock.core.base_resource;
import hammock.core.swapchain;



namespace hammock::core {
    export class FrameManager: public Singleton<FrameManager> {
        friend class Singleton<FrameManager>;

       public:
        ~FrameManager();

        static void initialize(BaseSurfaceProvider& i_surfaceProvider, Device& device) {
            Singleton<FrameManager>::initialize(i_surfaceProvider, device);
        }

        // delete copy constructor and copy destructor
        FrameManager(const FrameManager&) = delete;
        FrameManager& operator=(const FrameManager&) = delete;

        [[nodiscard]] SwapChain* getSwapChain() const { return swapChain.get(); };
        [[nodiscard]] float getAspectRatio() const { return swapChain->extentAspectRatio(); }
        [[nodiscard]] bool isFrameInProgress() const { return isFrameStarted; }

        [[nodiscard]] int getFrameIndex() const {
            if(!isFrameStarted)
                throw std::runtime_error("Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        [[nodiscard]] int getSwapChainImageIndex() const {
            if(!isFrameStarted)
                throw std::runtime_error("Cannot get image index when frame not in progress");
            return currentImageIndex;
        }

        void submitPresentCommandBuffer(VkCommandBuffer commandBuffer, const std::vector<VkSemaphore>& wait,
            const std::vector<VkPipelineStageFlags>& waitStages);

        bool beginFrame();

        void endFrame();

       protected:
        FrameManager(BaseSurfaceProvider& i_surfaceProvider, Device& device);

        void recreateSwapChain();

        BaseSurfaceProvider& surfaceProvider;
        Device& device;
        std::unique_ptr<SwapChain> swapChain;
        std::vector<ResourceHandle> handlesOfSwapImages;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted{false};
    };
}  // namespace hammock::core
