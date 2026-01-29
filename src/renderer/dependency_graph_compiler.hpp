#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "dependency_graph.hpp"
#include "gpu_task.hpp"

namespace hammock::renderer {

    struct CompiledLogicalResource final {
        /// Original resource handle for debugging
        LogicalResourceHandle origin;
        /// Handles of physical resources (may be multiple for resources that need to have a copy for each
        /// frame in flight)
        std::vector<core::ResourceHandle> handles{};
    };

    struct CompiledLogicalResourceAccess {
        /// Compiled logical resource
        CompiledLogicalResource* compiledLogicalResource{nullptr};

        /// Binding information (one of DescriptorBinding or AttachmentLocation)
        BindingInterface bindingIface{};

        /// Required layout (if image)
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
        /// Access flags
        vk::AccessFlagBits2 access;
        /// Stages that use the resource
        vk::PipelineStageFlagBits2 stages;

        /// What kind of resource
        vk::DescriptorType descriptorType;
    };

    enum class CompiledTaskType { Compute, Graphics };

    struct DispatchInfo {
        // TODO
    };

    struct DrawInfo {
        // TODO
    };

    /// @struct CompiledTask
    /// @brief Represents a GPU task that has been compiled by the graph compiler.
    /// Usually lives inside CompiledGraph
    struct CompiledTask final {
        /// Original task before compilation (for debug)
        TaskHandle origin;
        /// Type of task
        CompiledTaskType type;
        /// Constructed pipeline
        std::unique_ptr<core::BasePipeline> pipeline{nullptr};

        /// Compiled sockets
        std::vector<CompiledLogicalResourceAccess> compiledResourceAccesses{};

        /// Image barriers that need to be applied before this task executes
        std::vector<vk::ImageMemoryBarrier2> imageBarriers{};
        /// Buffer barriers that need to be applied before this task executes
        std::vector<vk::BufferMemoryBarrier2> bufferBarriers{};

        /// Src stages
        vk::PipelineStageFlagBits2 srcStages;
        /// Dst stages
        vk::PipelineStageFlagBits2 dstStages;

        // Execution parameters
        /// Dispatch info for compute tasks
        DispatchInfo dispatchInfo{};
        /// Drawing info for graphics tasks
        DrawInfo drawInfo{};
    };

    /// @struct CompiledDependencyGraph
    /// @brief Represents output of graph compiler
    struct CompiledDependencyGraph final {
        std::vector<CompiledLogicalResource> compiledLogicalResources{};
        std::vector<CompiledTask> compiledTasks{};
        std::vector<std::vector<std::uint32_t>> executionLevels{};
    };

    /// @struct TaskNode
    /// @brief Internal node representation for graph compilation
    struct TaskNode {
        BaseGpuTask* task = nullptr;
        std::vector<std::uint32_t> outgoing{};
        std::uint32_t indegree = 0;
    };

    enum class ReadWriteIntent { Read, Write, ReadWrite };

    /// @class DependencyGraphCompiler
    /// @brief Outputs compiled graph
    class DependencyGraphCompiler final {
        struct TaskHandleLogicalResourceAccessPair {
            TaskHandle task;
            AccessInterface access;
        };

        std::vector<TaskNode> nodes_;
        std::unordered_map<LogicalResourceHandle, std::vector<TaskHandleLogicalResourceAccessPair>,
            LogicalResourceHandleHash>
            logicalResourceUses_;
        CompiledDependencyGraph compiledDependencyGraph_{};


        /// Populates nodes_ list
        void buildGraphNodes(DependencyGraph& dependencyGraph);
        void addEdge(std::uint32_t aidx, std::uint32_t bidx);
        ReadWriteIntent determineReadWriteIntent(AccessInterface accessIface);
        bool isHazard(ReadWriteIntent a, ReadWriteIntent b);
        void handleExplicitDependencies(DependencyGraph& dependencyGraph);
        

        /// Creates physical resources from logical resources and wraps them in compiled logical resources
        void compileLogicalResources(DependencyGraph& dependencyGraph);
        core::ResourceHandle createPhysicalImageResource(LogicalImageResource* logicalImageResource);
        core::ResourceHandle createPhysicalBufferResource(LogicalBufferResource* logicalBufferResource);

        /// Assert debug edges
        void assertDebugEdges(DependencyGraph& dependencyGraph);

        /// Determine execution levels
        /// Task is ready if indegree == 0
        /// Ready task does not need to wait for any other tasks to finish
        /// Tasks in the same execution level can run in parallel
        void determineExecutionLevels();

       public:
        /// @brief Compiles the dependency. Result can be executed by DependencyGraphExecutor
        CompiledDependencyGraph compileDependencyGraph(DependencyGraph& dependencyGraph) {
            compileLogicalResources(dependencyGraph);
            buildGraphNodes(dependencyGraph);
            determineExecutionLevels();
            return std::move(compiledDependencyGraph_);
        }
    };

}  // namespace hammock::renderer
