#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "dependency_graph.hpp"
#include "gpu_task.hpp"


namespace hammock::renderer {

    struct CompiledLogicalResource final {
        /// Original resource handle for debugging
        LogicalResourceHandle origin;
        /// Handles of physical resources (may be multiple for resources that need to have a copy for each frame in flight)
        std::vector<core::ResourceHandle> handles{};

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


    enum class CompiledTaskType {
        Compute, Graphics
    };

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
        std::vector<CompiledLogicalResource> sockets{};

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
        std::vector<CompiledTask> compiledTasks{};
    };

    /// @struct TaskNode
    /// @brief Internal node representation for graph compilation
    struct TaskNode {
        BaseGpuTask * task;
        std::vector<TaskNode *> outgoing{};
        std::uint32_t indegree = 0;
    };

    enum class ReadWriteIntent {
        Read,
        Write,
        ReadWrite
    };



    /// @class DependencyGraphCompiler
    /// @brief Outputs compiled graph
    class DependencyGraphCompiler final {
        std::vector<TaskNode> nodes_;
        CompiledDependencyGraph compiledDependencyGraph_{};


        void buildGraphNodes(DependencyGraph &dependencyGraph);

    public:

        /// @brief Compiles the dependency. Result can be executed by DependencyGraphExecutor
        CompiledDependencyGraph compileDependencyGraph(DependencyGraph &dependencyGraph) {


            return std::move(compiledDependencyGraph_);
        }
    };


}
