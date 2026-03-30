#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "command_buffer.hpp"
#include "image.hpp"
#include "resource_manager.hpp"
#include "semaphore.hpp"
#include "utils/thread_pool.hpp"
namespace hammock::renderer {

    struct PassTransitions {
        std::vector<core::ImageMemoryBarrier> images;
        std::vector<core::BufferMemoryBarrier> buffers;
    };

    struct GraphicsPassDesc {
        std::string name;
        PassTransitions transitions;
        std::vector<core::ResourceRef<core::Image>> colorAttachments;
        std::optional<core::ResourceRef<core::Image>> depthAttachment;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct ComputePassDesc {
        std::string name;
        PassTransitions transitions;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct TransferPassDesc {
        std::string name;
        PassTransitions transitions;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    enum class RecordingStrategy { Singlethreaded, Multithreaded };

    struct RenderBatchDesc {
        std::vector<GraphicsPassDesc> graphics;
        std::vector<ComputePassDesc> compute;
        std::vector<TransferPassDesc> transfer;
        RecordingStrategy strategy;
        std::optional<uint64_t> singal;  // signal value
        std::optional<uint64_t> wait;    // wait value
        std::optional<core::ResourceRef<core::Semaphore>> semaphore;
        core::PipelineStage waitStage = core::PipelineStage::Invalid;
    };

    struct FrameGraphDesc {
        std::vector<RenderBatchDesc> batches;
        core::ResourceManager<core::CommandBuffer>& commandBufferManager;
        threading::ThreadPool& threadPool;
        std::optional<core::ResourceRef<core::CommandPool>> graphicsPool;
        std::optional<core::ResourceRef<core::CommandPool>> computePool;
        std::optional<core::ResourceRef<core::CommandPool>> transferPool;

    };

    class FrameGraph {
       public:
        explicit FrameGraph(FrameGraphDesc&& desc);

        void execute();

       private:
        void recordBarrier(PassTransitions& transitions, core::ResourceRef<core::CommandBuffer> cmd);

        FrameGraphDesc desc_;
    };

}  // namespace hammock::renderer
