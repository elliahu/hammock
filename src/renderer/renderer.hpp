#pragma once
#include <memory>
#include <stdexcept>

#include "core/command_buffer.hpp"
#include "core/vulkan_context.hpp"

namespace hammock::renderer {
    class Renderer {
       public:
        explicit Renderer(core::VulkanContext& context);

        void drawFrame(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore& signal) const;

       private:
        core::VulkanContext& ctx_;
        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers_;
    };
};  // namespace hammock::renderer
