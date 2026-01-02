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
    export using OnSwapChainRecreatedCallback = std::function<void(std::uint32_t, std::uint32_t)>;

    export class SwapChainManager : public Singleton<SwapChainManager> {
        friend class Singleton<SwapChainManager>;

    public:
        static void initialize(BaseSurfaceProvider &i_surfaceProvider, Device &device) {
            Singleton<SwapChainManager>::initialize(i_surfaceProvider, device);
        }

        // delete copy constructor and copy destructor
        SwapChainManager(const SwapChainManager &) = delete;

        SwapChainManager &operator=(const SwapChainManager &) = delete;

        [[nodiscard]] SwapChain &getSwapChain() const { return *swapChain; };
        [[nodiscard]] bool isFrameInProgress() const { return isFrameStarted; }

        [[nodiscard]] int getFrameIndex() const {
            if (!isFrameStarted)
                throw std::runtime_error("Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        [[nodiscard]] int getSwapChainImageIndex() const {
            if (!isFrameStarted)
                throw std::runtime_error("Cannot get image index when frame not in progress");
            return currentImageIndex;
        }

        void present();

        bool beginFrame();

        void endFrame();

        void registerOnSwapChainRecreatedCallback(const OnSwapChainRecreatedCallback &callback) {
            onSwapChainRecreated.push_back(std::move(callback));
        }

    protected:
        SwapChainManager(BaseSurfaceProvider &i_surfaceProvider, Device &device);

        void recreateSwapChain();

        BaseSurfaceProvider &surfaceProvider;
        Device &device;
        std::unique_ptr<SwapChain> swapChain;

        std::uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted{false};

        std::vector<OnSwapChainRecreatedCallback> onSwapChainRecreated{};
    };
} // namespace hammock::core
