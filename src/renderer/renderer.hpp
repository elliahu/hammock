#pragma once
#include <cstdint>
#include <functional>
#include <memory>

#include "core/command_buffer.hpp"
#include "core/descriptors.hpp"
#include "core/device.hpp"
#include "core/image.hpp"
#include "core/instance.hpp"
#include "core/pipeline.hpp"
#include "core/resource_manager.hpp"
#include "core/semaphore.hpp"
#include "render_proxy.hpp"
#include "render_types.hpp"
#include "vulkan/vulkan.hpp"

namespace hammock::renderer {

    /// @brief Interface for a renderer
    class RendererIface {
       public:
        virtual ~RendererIface() = default;

        /// @brief Draw frame onto a target
        virtual void drawFrame(core::ResourceRef<core::Image> target, RenderSnapshot& snap,
            core::ResourceRef<core::Semaphore> signal) = 0;

        /// @brief Get the device associated with this renderer
        virtual core::Device& getDevice() = 0;

        /// @brief Wait for all in-flight frames to complete
        virtual void waitIdle() = 0;

        /// @brief Handle a resize event
        virtual void handleResize(RenderCommand resizeCmd) = 0;
    };

    /// @brief Default implementation of a renderer
    class Renderer : public RendererIface {
       public:
        using SurfaceFactory = std::function<vk::SurfaceKHR(core::Instance&)>;
        using SurfaceDestructor = std::function<void(core::Instance&, vk::SurfaceKHR)>;
        explicit Renderer(SurfaceFactory surfaceFactory, SurfaceDestructor surfaceDestructor);

        ~Renderer() override;

        /// @brief Draw frame onto a target
        /// @param target Target image
        /// @param signal Semaphore reference, will be signaled when frame is finished rendering
        void drawFrame(core::ResourceRef<core::Image> target, RenderSnapshot& snap,
            core::ResourceRef<core::Semaphore> signal) override;

        /// @brief Get the device associated with this renderer
        core::Device& getDevice() override { return device_; }

        /// @brief Wait for the device to become idle
        /// You can use this to synchronize with the GPU before shutting down.
        void waitIdle() override;

        /// @brief Handle a resize event
        void handleResize(RenderCommand resizeCmd) override {
            // TODO handle resize event here, recreate buffers etc.
        }

       private:
        core::Instance instance_;
        vk::SurfaceKHR surface_;
        SurfaceDestructor surfaceDestructor_{nullptr};
        core::Device device_;

        /// This function updates the frame index keeping it in range 0 -> MAX_FRAMES_IN_FLIGHT
        void nextFrameIdx();

        /// Index of the current frame
        std::uint32_t currentFrameIdx_ = 0;

        // Resource managers
        core::ResourceManager<core::CommandBuffer> commandBuffers_{};
        core::ResourceManager<core::Buffer> buffers_{};
        core::ResourceManager<core::Image> images_{};
        core::ResourceManager<core::DescriptorPool> descriptorPools_{};
        core::ResourceManager<core::Pipeline> pipelines_{};

        // pool
        core::Handle<core::DescriptorPool> descriptorPoolHandle_;

        // Command buffers
        std::vector<core::Handle<core::CommandBuffer>> cmds;

        /// Ui rendering
        struct UserInterfacePushConstants {
            Vec2 screenSize;
        };

        core::Handle<core::Image> fontAtlasHandle;

        core::Handle<core::Pipeline> userInterfacePipeline_;

        std::unique_ptr<core::DescriptorSetLayout> userInterfaceDescLayout_;
        core::DescriptorSet userInterfaceDescSet_;

        core::Handle<core::Buffer> userInterfaceVertexBuffer_;
    };
};  // namespace hammock::renderer
