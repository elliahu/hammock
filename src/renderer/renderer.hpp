#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>

#include "base_resource.hpp"
#include "core/command_buffer.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "pipeline.hpp"
#include "render_types.hpp"
#include "resource_manager.hpp"
#include "semaphore.hpp"
#include "vulkan/vulkan.hpp"

namespace hammock::renderer {

    class RendererIface {
       public:
        virtual ~RendererIface() = default;
        virtual void drawFrame(core::ResourceRef<core::Image> target, RenderSnapshot& snap,
            core::ResourceRef<core::Semaphore> signal) = 0;

        virtual core::Device& getDevice() = 0;
    };

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

        core::Device& getDevice() override { return device_; }

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
