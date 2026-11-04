#pragma once
#include <array>
#include <cstdint>
#include <functional>

#include "command_cache.hpp"
#include "core/command_buffer.hpp"
#include "core/descriptors.hpp"
#include "core/device.hpp"
#include "core/image.hpp"
#include "core/instance.hpp"
#include "core/pipeline.hpp"
#include "core/resource_manager.hpp"
#include "core/semaphore.hpp"
#include "render_snapshot.hpp"
#include "swapchain.hpp"
#include "utils/math.hpp"
#include "utils/thread_pool.hpp"
#include "vulkan/vulkan.hpp"

namespace hammock::renderer {

    /// @brief Interface for a renderer
    class RendererIface {
       public:
        virtual ~RendererIface() = default;

        /// @brief Draw frame onto a target
        /// @return A timeline semaphore point signaled when rendering is complete
        virtual core::TimelineSemaphorePoint drawFrame(
            core::ResourceRef<core::Image> target, RenderSnapshotHeader* snap, const void* snapData) = 0;

        /// @brief Get the device associated with this renderer
        virtual core::Device& getDevice() = 0;

        /// @brief Wait for all in-flight frames to complete
        virtual void waitIdle() = 0;

        /// @brief Handle a resize event
        virtual void handleResize(uint32_t w, uint32_t h) = 0;
    };

    /// @brief Default implementation of a renderer
    class Renderer : public RendererIface {
       public:
        using SurfaceFactory = std::function<vk::SurfaceKHR(core::Instance&)>;
        using SurfaceDestructor = std::function<void(core::Instance&, vk::SurfaceKHR)>;

        /// @brief Construct a renderer with a custom surface factory and destructor
        /// @param surfaceFactory Function to create the surface
        /// @param surfaceDestructor Function to destroy the surface
        explicit Renderer(SurfaceFactory surfaceFactory, SurfaceDestructor surfaceDestructor);

        /// @brief Destroys the renderer and release resources that are not released automatically
        ~Renderer() override;

        /// @brief Draw frame onto a target
        /// @param target Target image
        /// @return A timeline semaphore point signaled when rendering is complete
        core::TimelineSemaphorePoint drawFrame(core::ResourceRef<core::Image> target, RenderSnapshotHeader* snap,
            const void* snapData) override;

        /// @brief Get the device associated with this renderer
        core::Device& getDevice() override { return device_; }

        /// @brief Wait for the device to become idle
        /// You can use this to synchronize with the GPU before shutting down.
        void waitIdle() override;

        /// @brief Handle a resize event
        void handleResize(uint32_t w, uint32_t h) override {
            // TODO handle resize event here, recreate buffers etc.
        }

       private:
        /// This function updates the frame index keeping it in range 0 -> MAX_FRAMES_IN_FLIGHT
        void prepNextFrame();

        core::Instance instance_;
        vk::SurfaceKHR surface_;  // TODO abstract this somehow
        SurfaceDestructor surfaceDestructor_{
            nullptr
        };  // This is called in the destructor to destroy surface which is created externally
        core::Device device_;
        ThreadPool threadPool_{};
        CommandCache commandCache_;

        /// Index of the current frame
        uint32_t currentFrameIdx_ = 0;
        uint64_t computeSignalValue = 1;
        uint64_t graphicsSignalValue_ = 1;

        // Resource managers
        core::ResourceManager<core::CommandPool> commandPools_{};
        core::ResourceManager<core::CommandBuffer> commandBuffers_{};
        core::ResourceManager<core::Buffer> buffers_{};
        core::ResourceManager<core::Image> images_{};
        core::ResourceManager<core::DescriptorPool> descriptorPools_{};
        core::ResourceManager<core::DescriptorSetLayout> descriptorSetLayouts_{};
        core::ResourceManager<core::Pipeline> pipelines_{};
        core::ResourceManager<core::Semaphore> semaphores_{};

        // pools
        core::Handle<core::DescriptorPool> descriptorPoolHandle_;
        core::Handle<core::CommandPool> prepPoolHandle_;

        struct UserInterfacePushConstants {
            Vec2 screenSize;
        };

        struct ComputePushConstants {
            Vec4 color;
        };

        struct {
            core::Handle<core::Semaphore> graphics;
            core::Handle<core::Semaphore> compute;
            core::Handle<core::Semaphore> transfer;
        } timeline_;

        // Compute
        core::Handle<core::Pipeline> computePipelineHandle_;
        core::Handle<core::DescriptorSetLayout> computeLayoutHandle_;
        std::array<core::DescriptorSet, core::SwapChain::MAX_FRAMES_IN_FLIGHT> computeDescriptorSets_;

        // Ui
        core::Handle<core::Image> fontAtlasHandle;
        core::Handle<core::Pipeline> userInterfacePipelineHandle_;
        core::Handle<core::DescriptorSetLayout> userInterfaceDescLayoutHandle_;
        core::DescriptorSet userInterfaceDescSet_;
        std::array<core::Handle<core::Buffer>, core::SwapChain::MAX_FRAMES_IN_FLIGHT> userInterfaceVertexBuffers_;
    };
};  // namespace hammock::renderer
