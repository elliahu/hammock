#pragma once

#include "command_buffer.hpp"
#include "render_graph_compiler.hpp"
#include "resource_manager.hpp"
#include "swapchain.hpp"
#include "utils/thread_pool.hpp"

namespace hammock::graph {

    /// @brief One timeline semaphore per queue family, carrying a monotonically increasing counter. The
    /// executor allocates these once and passes them in.
    struct QueueTimeline {
        core::CommandQueueFamily family;
        VkSemaphore semaphore{VK_NULL_HANDLE};
    };

    /// @brief A planned command buffer submission using timeline semaphores.
    struct CommandBufferBatch {
        core::CommandQueueFamily family;
        std::vector<uint32_t> passes;

        // This batch signals its queue's timeline to `signalValue` on completion.
        uint64_t signalValue{0};

        // Before this batch is submitted, wait for each listed batch's signal.
        // The executor resolves batchIdx -> QueueTimeline + signalValue at submit time.
        struct SemaphoreWait {
            uint32_t batchIdx;  // index into the returned vector
            uint64_t value;     // value to wait for on that batch's timeline semaphore
        };
        std::vector<SemaphoreWait> waits;
    };

    struct ExecutionPlan {
        std::vector<CommandBufferBatch> batches;
    };

    class RenderGraphExecutionPlanner {
       public:
        /// @brief Create the execution plan
        ExecutionPlan plan(CompiledRenderGraph& graph);
    };

    /// @class RenderGraphExecutor
    /// @brief Executes compiled graph, orchestrates the recording and submission of command buffers, creates
    /// actuall GPU resources
    class RenderGraphExecutor final {
       public:
        RenderGraphExecutor(core::Device& device, ExecutionPlan& plan, CompiledRenderGraph& graph);

        /// @brief Executes the compiled dependency graph
        void execute();

       private:
        void nextFrameIdx();

        core::Device& device_;                // Current device
        ExecutionPlan& plan_;                 // Execution plan
        CompiledRenderGraph& graph_;          // Compiled render graph
        threading::ThreadPool threadPool_{};  // Threadpool to schedule the work
        uint32_t frameIdx{0};

        // Resource managers
        core::ResourceManager<core::CommandPool> commandPools_{};
        core::ResourceManager<core::CommandBuffer> commandBuffers_;

        // Resource handles
        struct PerFrameResources{
            std::vector<core::Handle<core::CommandBuffer>> commandBuffers;
        };
        std::vector<PerFrameResources> perFrameResources_;
        core::Handle<core::CommandPool> graphicsPool_;
        core::Handle<core::CommandPool> computePool_;
        core::Handle<core::CommandPool> transferPool_;

    };

}  // namespace hammock::graph
