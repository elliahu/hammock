#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "command_buffer.hpp"
#include "command_cache.hpp"
#include "device.hpp"
#include "image.hpp"
#include "resource_manager.hpp"
#include "semaphore.hpp"

namespace hammock::renderer {

    struct PassTransitionsInfo {
        std::vector<core::ImageMemoryBarrier> images;
        std::vector<core::BufferMemoryBarrier> buffers;
    };

    struct ImageAccessInfo {
        core::ResourceRef<core::Image> image;  // What image
        core::ResourceState state;             // What state is required
    };

    struct BufferAccessInfo {
        core::ResourceRef<core::Buffer> buffer;  // Buffer
        core::ResourceState state;               // What state is required
    };

    /// @struct PassResourceAccessInfo
    /// @note Reserved for future usage
    struct PassResourceAccessInfo {
        std::vector<ImageAccessInfo> images;    // Image accesses
        std::vector<BufferAccessInfo> buffers;  // Buffer accesses
    };

    struct PushConstantsInfo {
        core::ShaderStage stages;
        uint32_t size;
        const void* data;
    };

    struct PassDataInfo {
        std::vector<core::DescriptorSet> descriptors;
        PushConstantsInfo constants;
    };

    struct PassSyncSemaphoreInfo {
        uint64_t value;
        core::CommandQueueFamily queue;
        core::PipelineStage stage;
    };

    struct PassSyncInfo {
        std::vector<PassSyncSemaphoreInfo> waits;
        std::vector<PassSyncSemaphoreInfo> signals;
    };


    struct RenderingDescription {
        std::vector<core::ResourceRef<core::Image>> colorAttachments;
        std::optional<core::ResourceRef<core::Image>> depthAttachment;
        std::optional<core::ResourceRef<core::Image>> stencilAttachment;
        std::optional<core::Rect2D> area;
        std::optional<core::Viewport> viewport;
        std::optional<core::Rect2D> scissors;
        std::optional<core::ResourceRef<core::Pipeline>> pipeline;
    };

    struct DispatchGroupSizes {
        uint32_t x = 0, y = 0, z = 0;
    };

    struct DispatchDescription {
        DispatchGroupSizes sizes;
        std::optional<core::ResourceRef<core::Pipeline>> pipeline;
    };

    struct TransferDescription {};


    struct PassDescription {
        core::CommandQueueFamily family;
        PassTransitionsInfo transitions;
        PassDataInfo data;
        PassSyncInfo sync;
        TransferDescription transfer;
        DispatchDescription dispatch;
        RenderingDescription rendering;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct FrameGraphDescription {
        std::vector<PassDescription> passes;
    };

    /// @class FrameGraph
    class FrameGraph {
       public:
        explicit FrameGraph(FrameGraphDescription&& desc, CommandCache& commandCache,
            core::ResourceRef<core::Semaphore> graphicsTimeline,
            core::ResourceRef<core::Semaphore> computeTimeline,
            core::ResourceRef<core::Semaphore> transferTimeline);

        void execute(uint32_t frameIndex = 0);

       private:
        void recordBarrier(PassTransitionsInfo& transitions, core::ResourceRef<core::CommandBuffer> cmd);
        void recordGraphicsPass(PassDescription& pass, core::ResourceRef<core::CommandBuffer> cmd);
        void recordComputePass(PassDescription& pass, core::ResourceRef<core::CommandBuffer> cmd);

        FrameGraphDescription desc_;
        CommandCache& commandCache_;

        struct {
            core::ResourceRef<core::Semaphore> graphics;
            core::ResourceRef<core::Semaphore> compute;
            core::ResourceRef<core::Semaphore> transfer;
        } timeline_;
    };
}  // namespace hammock::renderer
