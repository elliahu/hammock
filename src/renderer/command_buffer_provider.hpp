#pragma once

#include <vector>

#include "core/command_buffer.hpp"
#include "core/device.hpp"
#include "core/resource_manager.hpp"

namespace hammock::renderer {

    /// @brief Command buffer pool local to a thread and frame
    struct LocalCommandBufferPool {
        core::Handle<core::CommandPool> pool;
        std::vector<core::Handle<core::CommandBuffer>> primaryCmds;
        std::vector<core::Handle<core::CommandBuffer>> secondaryCmds;
        uint32_t nextPrimary = 0;
        uint32_t nextSecondary = 0;
    };

    /// @brief Helper class for command buffer management
    /// TODO check if family is available on the device
    class CommandBufferProvider {
       public:
        CommandBufferProvider(core::Device& device, size_t frames, size_t threads);

        core::ResourceRef<core::CommandBuffer> getCommandBuffer(core::CommandQueueFamily family,
            core::CommandBufferLevel level, uint32_t frameIndex, uint32_t threadIndex);

        void resetPools(uint32_t frameIndex);

       private:
        core::ResourceRef<core::CommandBuffer> allocateOrReuse(
            LocalCommandBufferPool& pool, core::CommandBufferLevel level);

        core::Device& device_;
        size_t frameCount_, threadCount_;
        core::ResourceManager<core::CommandPool> pools_;
        core::ResourceManager<core::CommandBuffer> commandBuffers_;

        std::vector<std::vector<std::vector<LocalCommandBufferPool>>> frames_;
    };

};  // namespace hammock::renderer
