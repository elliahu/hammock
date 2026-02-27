#pragma once
#include <memory>
#include <stdexcept>

#include "base_resource.hpp"
#include "core/command_buffer.hpp"
#include "core/vulkan_context.hpp"
#include "descriptors.hpp"
#include "graphics_pipeline.hpp"
#include "render_types.hpp"
#include "semaphore.hpp"
#include "swapchain.hpp"

namespace hammock::renderer {

    class RendererIface {
       public:
        virtual ~RendererIface() = default;
        virtual void drawFrame(core::ResourceHandle target, RenderSnapshot& snap, core::Semaphore& signal) = 0;
    };

    class Renderer : public RendererIface{
       public:
        explicit Renderer(core::VulkanContext& context);

        /// @brief Draw frame onto a target
        /// @param target Target image
        /// @param signal Semaphore reference, will be signaled when frame is finished rendering
        void drawFrame(core::ResourceHandle target, RenderSnapshot& snap,  core::Semaphore& signal);

       private:
        /// This function updates the frame index keeping it in range 0 -> MAX_FRAMES_IN_FLIGHT
        void nextFrameIdx();

        /// Index of the current frame
        std::uint32_t currentFrameIdx_ = 0;

        /// Vulkan context
        core::VulkanContext& ctx_;

        // Command buffers
        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers_;

        /// Ui rendering
        struct UserInterfacePushConstants{
            Vec2 screenSize;
        };
        core::ResourceHandle fontAtlasHandle;

        std::unique_ptr<core::GraphicsPipeline> userInterfacePipeline_;

        std::unique_ptr<core::DescriptorSetLayout> userInterfaceDescLayout_;
        core::DescriptorSet userInterfaceDescSet_;

        core::ResourceHandle userInterfaceVertexBuffer_;
    };
};  // namespace hammock::renderer
