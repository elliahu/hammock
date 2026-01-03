module;
#include <stdexcept>
#include <vector>
#include <memory>
#include <functional>

export module hammock.core.swapchain_manager;

import hammock.core.device;
import hammock.core.utilities;
import hammock.core.base_surface_provider;
import hammock.core.base_resource;
import hammock.core.swapchain;


namespace hammock::core {
    /// @brief SwapChain recreated callback type
    export using OnSwapChainRecreatedCallback = std::function<void(std::uint32_t, std::uint32_t)>;

    /// @class SwapChainManager
    /// @brief Manager responsible for manipulation the SwapChain
    export class SwapChainManager final : public Singleton<SwapChainManager> {
        friend class Singleton<SwapChainManager>;

    public:
        static void initialize(BaseSurfaceProvider &i_surfaceProvider, Device &device);

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

    protected:
        SwapChainManager(BaseSurfaceProvider &i_surfaceProvider, Device &device);

        void recreateSwapChain();

        BaseSurfaceProvider &surfaceProvider_;
        Device &device_;
        std::unique_ptr<SwapChain> swapChain_;

        std::uint32_t currentImageIndex_;
        int currentFrameIndex_{0};
        bool isFrameStarted_{false};

        std::vector<OnSwapChainRecreatedCallback> onSwapChainRecreated_{};
    };
} // namespace hammock::core
