#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "command_buffer.hpp"
#include "device.hpp"
#include "image.hpp"
#include "resource_manager.hpp"
#include "semaphore.hpp"
#include "utils/thread_pool.hpp"
namespace hammock::renderer {

    struct PassTransitions {
        std::vector<core::ImageMemoryBarrier> images;
        std::vector<core::BufferMemoryBarrier> buffers;
    };

    struct PushConstants {
        core::ShaderStage stages;
        uint32_t size;
        const void* data;
    };

    struct PassDataInfo{
        std::vector<core::DescriptorSet> descriptors;
        PushConstants constants;
    };



    struct GraphicsPassRenderingInfo {
        std::vector<core::ResourceRef<core::Image>> colorAttachments;
        std::optional<core::ResourceRef<core::Image>> depthAttachment;
        std::optional<core::ResourceRef<core::Image>> stencilAttachment;
        std::optional<core::Rect2D> area;
        std::optional<core::Viewport> viewport;
        std::optional<core::Rect2D> scissors;
        std::optional<core::ResourceRef<core::Pipeline>> pipeline;
    };

    struct GraphicsPassDesc {
        std::string name;
        PassTransitions transitions;
        GraphicsPassRenderingInfo rendering;
        PassDataInfo data;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct ComputePassDispatchInfo {
        std::vector<core::DescriptorSet> descriptors;
        PushConstants constants;
    };

    struct ComputePassDesc {
        std::string name;
        PassTransitions transitions;
        ComputePassDispatchInfo dispatch;
        PassDataInfo data;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct TransferPassDesc {
        std::string name;
        PassTransitions transitions;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct RenderBatchDesc {
        std::vector<GraphicsPassDesc> graphics;
        std::vector<ComputePassDesc> compute;
        std::vector<TransferPassDesc> transfer;
        std::optional<uint64_t> singal;  // signal value
        std::optional<uint64_t> wait;    // wait value
        std::optional<core::ResourceRef<core::Semaphore>> semaphore;
        core::PipelineStage waitStage = core::PipelineStage::Invalid;
    };

    class CommandBufferCache {
       public:
        // Layz init
        void init(core::Device& device, core::CommandQueueFamily family, uint32_t numThreads);
        // Called at the start of the frame
        void reset();
        // Get a primary command buffer for the main thread
        core::ResourceRef<core::CommandBuffer> getPrimaryCommandBuffer();
        // Get a secondary command buffer for a specific worker thread
        core::ResourceRef<core::CommandBuffer> getSecondaryCommandBuffer(uint32_t threadIndex);

       private:
        // Managers
        core::ResourceManager<core::CommandPool> commandPools_;
        core::ResourceManager<core::CommandBuffer> commandBuffers_;
        // Main thread pool and buffers
        core::Handle<core::CommandPool> mainPoolHndl_;
        core::Handle<core::CommandBuffer> primaryBufferHndl_;

        // Worker thread pools and buffers
        struct ThreadCache {
            core::Handle<core::CommandBuffer> secondaryBufferHdnl;
        };
        std::vector<ThreadCache> threadCaches_;
    };

    struct FrameGraphDesc {
        std::vector<RenderBatchDesc> batches;
        CommandBufferCache& commandCache;
        threading::ThreadPool& threadPool;
    };

    class FrameGraph {
       public:
        explicit FrameGraph(FrameGraphDesc&& desc);

        void execute();

       private:
        void recordBarrier(PassTransitions& transitions, core::ResourceRef<core::CommandBuffer> cmd);
        void recordGraphicsPass(GraphicsPassDesc& pass, core::ResourceRef<core::CommandBuffer> cmd);

        FrameGraphDesc desc_;
    };

}  // namespace hammock::renderer
