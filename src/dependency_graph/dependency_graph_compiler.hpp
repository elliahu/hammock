#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "core/base_pipeline.hpp"
#include "dependency_graph.hpp"
#include "core/descriptors.hpp"
#include "device.hpp"
#include "gpu_task.hpp"
#include "core/vulkan_context.hpp"

namespace hammock::graph {

    /// @struct CompiledLogicalResource
    /// @brief Represents compiled resource that is linked to the physical resources
    struct CompiledLogicalResource final {
        /// Original resource handle for debugging
        LogicalResourceHandle origin;
        /// Handles of physical resources (may be multiple for resources that need to have a copy for each
        /// frame in flight)
        std::vector<core::ResourceHandle> handles{};
        /// Persistent resource keeps its content across frames.
        /// Non-persistent resource may be cleared after each frame
        bool persistent = true;
    };

    /// @struct CompiledTask
    /// @brief Represents a GPU task that has been compiled by the graph compiler.
    /// Usually lives inside CompiledGraph
    struct CompiledTask final {
        /// Original task before compilation (for debug)
        TaskHandle origin;

        /// Type of task
        core::CommandQueueFamily family;

        /// Execution function
        TaskExecutionFunction execFunc{nullptr};

        /// Compiled resource accesses
        std::vector<uint32_t> compiledResourceAccesses{};

        /// Image barriers that need to be applied before this task executes
        std::vector<vk::ImageMemoryBarrier2> imageBarriers{};

        /// Buffer barriers that need to be applied before this task executes
        std::vector<vk::BufferMemoryBarrier2> bufferBarriers{};
    };

    /// @struct CompiledDependencyGraph
    /// @brief Represents output of graph compiler
    struct CompiledDependencyGraph final {
        std::vector<CompiledLogicalResource> compiledLogicalResources{};
        std::vector<CompiledTask> compiledTasks{};
        std::vector<std::vector<std::uint32_t>> executionLevels{};
        std::int32_t rootIdx = -1;
    };

    /// @struct TaskNode
    /// @brief Internal node representation for graph compilation
    struct TaskNode {
        GpuTask* task = nullptr;
        std::vector<std::uint32_t> outgoing{};
        std::vector<std::uint32_t> incoming{};
        std::uint32_t indegree = 0;
    };

    /// @enum ReadWriteIntent
    enum class ReadWriteIntent { Read, Write, ReadWrite };

    /// @class DependencyGraphCompiler
    /// @brief Outputs compiled graph
    class DependencyGraphCompiler final {
        core::VulkanContext& ctx_;
        /// Internal structure to pair up task and access
        struct TaskHandleLogicalResourceAccessPair {
            TaskHandle task;
            AccessInterface access;
        };

        std::vector<TaskNode> nodes_;  /// Intermediate graph structure
        std::unordered_map<LogicalResourceHandle, std::vector<TaskHandleLogicalResourceAccessPair>,
            LogicalResourceHandleHash>
            logicalResourceUses_;  /// Stores where is each resources accessed and how
        CompiledDependencyGraph
            compiledDependencyGraph_{};  /// The final compiled graph that will be returned

        // Populates nodes_ list
        void buildGraphNodes(DependencyGraph& dependencyGraph);
        void addEdge(std::uint32_t aidx, std::uint32_t bidx);
        ReadWriteIntent determineReadWriteIntent(AccessInterface accessIface);
        bool isHazard(ReadWriteIntent a, ReadWriteIntent b);
        void handleExplicitDependencies(DependencyGraph& dependencyGraph);

        // Creates physical resources from logical resources and wraps them in compiled logical resources
        void compileLogicalResources(DependencyGraph& dependencyGraph);
        core::ResourceHandle createPhysicalImageResource(LogicalImageResource* logicalImageResource);
        core::ResourceHandle createPhysicalBufferResource(LogicalBufferResource* logicalBufferResource);

        /// Assert debug edges
        void assertDebugEdges(DependencyGraph& dependencyGraph);

        /// Determine execution levels.
        /// Task is ready if indegree == 0.
        /// Ready task does not need to wait for any other tasks to finish.
        /// Tasks in the same execution level can run in parallel.
        void determineExecutionLevels();

        // Task compilation
        void compileTasks(DependencyGraph& dependencyGraph);
        void compileBarrier(
            const LogicalResourceAccess& prev, const LogicalResourceAccess& curr, CompiledTask& dstTask);
        vk::PipelineStageFlags2 stagesFromAccess(AccessInterface accessIface);
        vk::AccessFlags2 accessFlagsFromAccess(AccessInterface accessIface);
        vk::ImageLayout layoutFromAccess(ImageAccess access);
        void createComputePipeline(CompiledTask& task);

       public:
        DependencyGraphCompiler(core::VulkanContext& ctx);
        /// @brief Compiles the dependency. Result can be executed by DependencyGraphExecutor
        [[nodiscard]] CompiledDependencyGraph compile(DependencyGraph& dependencyGraph);
    };

}  // namespace hammock::renderer
