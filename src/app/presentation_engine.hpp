#pragma once
#include <memory>
#include <optional>
#include <compare>
#include <vulkan/vulkan.hpp>
#include <functional>

#include "core/semaphore.hpp"
#include "core/command_buffer.hpp"
#include "core/swapchain_manager.hpp"
#include "framebuffer.hpp"
#include "core/vulkan_context.hpp"
#include "core/surface_provider_iface.hpp"


namespace hammock::app {
    /// @brief Context provided to the renderer for each frame
    struct FrameContext {
        core::ResourceHandle renderTarget;
        core::Semaphore &renderFinished;
        uint32_t frameIndex;
    };

    // Base Strategy - Contains SHARED rendering orchestration

    /// @class BasePresentationStrategy
    /// @brief Base class containing common frame orchestration logic
    /// Uses Template Method pattern - subclasses implement presentation-specific hooks
    class BasePresentationStrategy {
    public:
        virtual ~BasePresentationStrategy() = default;

        // Public Interface (called by PresentationEngine)

        std::optional<FrameContext> beginFrame();

        void endFrame(const FrameContext &ctx);

        [[nodiscard]] vk::Extent2D getResolution() const;

        core::ImageFormat getFormat() const;

        std::uint32_t getFrameIndex() const { return currentFrameIndex_; }
        bool isFrameInProgress() const { return frameInProgress_; }

        void onResolutionChanged(std::function<void(uint32_t, uint32_t)> callback) {
            resolutionCallbacks_.push_back(std::move(callback));
        }

        virtual bool surfaceCapable() = 0;

    protected:
        // Protected Constructor (only subclasses can instantiate)
        BasePresentationStrategy(
            core::VulkanContext &ctx,
            vk::Extent2D resolution,
            core::ImageFormat format,
            uint32_t framesInFlight
        );

        // Template Method Hooks (subclasses override these)

        /// @brief Called at the start of beginFrame, before common logic
        /// @return false to skip this frame (e.g., swapchain out of date)
        virtual bool onBeginFrame() { return true; }

        /// @brief Called at the end of endFrame, after rendering is done
        /// Subclasses implement presentation logic here
        virtual void onPresent(const FrameContext &ctx) = 0;

        /// @brief Called when framebuffer needs recreation
        virtual void onFramebufferRecreated() {
        }

        // Protected Helpers (for subclass use)

        void recreateFramebuffer(vk::Extent2D newResolution);

        void notifyResolutionChanged(uint32_t width, uint32_t height);

        struct PerFrameResources {
            std::unique_ptr<core::Semaphore> renderingFinished;
        };

        // Protected Members (accessible by subclasses)

        core::VulkanContext &ctx_;
        std::unique_ptr<Framebuffer> framebuffer_;
        std::vector<PerFrameResources> perFrameResources_;

        uint32_t currentFrameIndex_ = 0;
        uint32_t framesInFlight_;
        bool frameInProgress_ = false;

        std::vector<std::function<void(uint32_t, uint32_t)> > resolutionCallbacks_;
    };

    // Concrete Strategies - Only implement presentation differences

    /// @class SurfacePresentationStrategy
    /// @brief Surface-based rendering - framebuffer + swapchain presentation
    class SurfacePresentationStrategy : public BasePresentationStrategy {
    public:
        enum class Mode {
            Tooling, 
            Game 
        };

        SurfacePresentationStrategy(
            core::VulkanContext &ctx,
            core::SurfaceProviderIface &surfaceProvider,
            Mode mode = Mode::Tooling
        );

        ~SurfacePresentationStrategy() override = default;

        /// @brief Submit UI rendering commands (Editor mode only)
        void submitUI(const FrameContext &ctx,
                      std::function<void(core::ResourceHandle, std::uint32_t, core::Semaphore &, core::Semaphore &)>
                      uiRenderFunc);

        /// @brief Get the swapchain image view for direct rendering
        vk::ImageView getSwapchainImageView(uint32_t index) const;

        bool surfaceCapable() override { return true; }

    protected:
        // Acquire swapchain image before common frame logic
        bool onBeginFrame() override;

        // Blit to swapchain and present
        void onPresent(const FrameContext &ctx) override;

        // Recreate framebuffer when swapchain changes
        void onFramebufferRecreated() override;

    private:
        void blitFramebufferToSwapchain(const FrameContext &ctx);

        Mode mode_;
        std::unique_ptr<core::SwapChainManager> swapchainManager_;

        // Additional per-frame resources for surface presentation
        struct SurfacePerFrameResources {
            std::unique_ptr<core::CommandBuffer> presentCommandBuffer;
            // Editor mode only
            std::unique_ptr<core::Semaphore> uiFinished;
        };

        std::vector<SurfacePerFrameResources> surfacePerFrameResources_;

        uint32_t currentSwapchainImageIndex_ = 0;
    };

    /// @class PresentationEngine
    /// @brief Main presentation engine - delegates to strategy
    class PresentationEngine {
    public:
        explicit PresentationEngine(std::unique_ptr<BasePresentationStrategy> &&strategy)
            : strategy_(std::move(strategy)) {
        }

        // Delegate all calls to strategy
        [[nodiscard]] std::optional<FrameContext> beginFrame() { return strategy_->beginFrame(); }

        void endFrame(const FrameContext &ctx) { strategy_->endFrame(ctx); }

        [[nodiscard]] vk::Extent2D getResolution() { return strategy_->getResolution(); }

        [[nodiscard]] core::ImageFormat getFormat() { return strategy_->getFormat(); }

        [[nodiscard]] bool isFrameInProgress() { return strategy_->isFrameInProgress(); }

        [[nodiscard]] std::uint32_t getFrameIndex() { return strategy_->getFrameIndex(); }

        void onResolutionChanged(std::function<void(uint32_t, uint32_t)> cb) {
            strategy_->onResolutionChanged(std::move(cb));
        }

        // Strategy-specific access (use with caution)
        [[nodiscard]] bool surfaceCapable() const{
            return strategy_->surfaceCapable();
        }

        template<typename T>
        T *getStrategyAs();

        template<typename T>
        const T *getStrategyAs() const;

    private:
        std::unique_ptr<BasePresentationStrategy> strategy_;
    };

    template<typename T>
    T *PresentationEngine::getStrategyAs() { return dynamic_cast<T *>(strategy_.get()); }

    template<typename T>
    const T *PresentationEngine::getStrategyAs() const { return dynamic_cast<const T *>(strategy_.get()); }
}
