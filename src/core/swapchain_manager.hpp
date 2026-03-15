#pragma once
#include <stdexcept>
#include <vector>
#include <memory>
#include <functional>

#include "device.hpp"
#include "utilities.hpp"
#include "surface_provider_iface.hpp"
#include "base_resource.hpp"
#include "swapchain.hpp"
#include "command_buffer.hpp"
#include "image.hpp"



namespace hammock::core {
    /// @brief SwapChain recreated callback type
    using OnSwapChainRecreatedCallback = std::function<void(std::uint32_t, std::uint32_t)>;

    /// @class SwapChainManager
    /// @brief Manager responsible for manipulation the SwapChain
    class SwapChainManager final{
    public:
        SwapChainManager(SurfaceProviderIface &i_surfaceProvider, Device &device);

        // delete copy constructor and copy destructor
        SwapChainManager(const SwapChainManager &) = delete;

        SwapChainManager &operator=(const SwapChainManager &) = delete;

        [[nodiscard]] SwapChain &getSwapChain() const { return *swapChain_; };
        [[nodiscard]] bool isFrameInProgress() const { return isFrameStarted_; }

        [[nodiscard]] int getFrameIndex() const;

        [[nodiscard]] int getSwapChainImageIndex() const;

        void present();

        bool beginFrame();

        void endFrame();

        void registerOnSwapChainRecreatedCallback(const OnSwapChainRecreatedCallback &callback);

        /// @brief Performs blit operation from src to current swapchain image
        /// @pre src is in transfer src layout
        /// @post swapchain image will be in present optimal layout
        void blitToSwapChainImage(ResourceRef<CommandBuffer> commandBuffer, ResourceRef<Image> image);

    protected:

        void recreateSwapChain();

        SurfaceProviderIface &surfaceProvider_;
        Device &device_;
        std::unique_ptr<SwapChain> swapChain_;

        std::uint32_t currentImageIndex_;
        int currentFrameIndex_{0};
        bool isFrameStarted_{false};

        std::vector<OnSwapChainRecreatedCallback> onSwapChainRecreated_{};
    };
} // namespace hammock::core
