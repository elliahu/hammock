#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

#include "core/command_buffer.hpp"
#include "core/semaphore.hpp"
#include "core/surface_provider_iface.hpp"
#include "core/swapchain_manager.hpp"
#include "framebuffer.hpp"
#include "resource_manager.hpp"
#include "utils/math.hpp"

namespace hammock::renderer {

    /// @struct FrameContext
    /// @brief Contains the resources needed for a single frame presentation.
    struct FrameContext {
        core::ResourceRef<core::Image> renderTarget;
        core::ResourceRef<core::Semaphore> renderFinished;
        uint32_t frameIndex;
    };

    /// @class IPresenter
    /// @brief Interface for a presenter that handles frame presentation.
    class PresenterIface {
       public:
        virtual ~PresenterIface() = default;

        virtual std::optional<FrameContext> beginFrame() = 0;
        virtual void endFrame(const FrameContext& ctx) = 0;

        [[nodiscard]] virtual vk::Extent2D getResolution() const = 0;
        [[nodiscard]] virtual core::ImageFormat getFormat() const = 0;
        [[nodiscard]] virtual bool isFrameInProgress() const = 0;
        [[nodiscard]] virtual uint32_t getFrameIndex() const = 0;

        virtual void onResolutionChanged(std::function<void(uint32_t, uint32_t)> callback) = 0;
    };

    /// @enum PresenterFrameBufferSize
    /// @brief Specifies the size of the internal framebuffer used by the presenter.
    enum class PresenterFrameBufferSize { SwapchainRelative, Absolute };

    /// @struct PresenterDesc
    /// @brief Describes the configuration for creating a Presenter.
    struct PresenterDesc {
        PresenterFrameBufferSize frameBufferRelativeSize = PresenterFrameBufferSize::SwapchainRelative;
        math::Vec2 frameBufferSize = {1.f, 1.f}; // Relative to the setting in frameBufferRelativeSize
    };

    /// @class SurfacePresenter
    /// @brief Implements the PresenterIface interface using a Vulkan swapchain for frame presentation.
    /// Presenter creates internal framebuffer that is the size of the surface (window) and blits it to the
    /// swapchain. This way both can be resized independently.
    class Presenter final : public PresenterIface {
       public:
        Presenter(core::Device& device, core::SurfaceProviderIface& surfaceProvider, const PresenterDesc& desc);
        ~Presenter() override = default;

        /// @brief Begins a new frame for rendering.
        /// @return An optional FrameContext if a new frame was successfully started, otherwise std::nullopt.
        std::optional<FrameContext> beginFrame() override;

        /// @brief Ends the current frame, presenting it to the swapchain.
        void endFrame(const FrameContext& ctx) override;

        [[nodiscard]] vk::Extent2D getResolution() const override;
        [[nodiscard]] core::ImageFormat getFormat() const override;
        [[nodiscard]] bool isFrameInProgress() const override { return frameInProgress_; }
        [[nodiscard]] uint32_t getFrameIndex() const override { return currentFrameIndex_; }

        void onResolutionChanged(std::function<void(uint32_t, uint32_t)> callback) override;

       private:
        void recreateFramebuffer(vk::Extent2D newResolution);
        void blitFramebufferToSwapchain(const FrameContext& ctx);
        void notifyResolutionChanged(uint32_t width, uint32_t height);
        math::Vec2 getFramebufferSize() const;

        core::Device& device_;
        std::unique_ptr<Framebuffer> framebuffer_;
        std::unique_ptr<core::SwapChainManager> swapchainManager_;

        struct PerFrameResources {
            core::Handle<core::Semaphore> renderingFinished;
            core::Handle<core::CommandBuffer> presentCommandBuffer;
        };

        PresenterDesc desc_;

        core::ResourceManager<core::Semaphore> semaphores_{};
        core::ResourceManager<core::CommandBuffer> commandBuffers_{};
        std::vector<PerFrameResources> perFrameResources_;

        uint32_t currentFrameIndex_ = 0;
        uint32_t currentSwapchainImageIndex_ = 0;
        uint32_t framesInFlight_;
        bool frameInProgress_ = false;

        std::vector<std::function<void(uint32_t, uint32_t)>> resolutionCallbacks_;
    };

}  // namespace hammock::renderer
