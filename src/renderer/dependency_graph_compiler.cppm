module;

#include <cstdint>
#include <vector>
#include <memory>
#include <vulkan/vulkan.hpp>

export module hammock_renderer:dependency_graph_compiler;

import :dependency_graph;

namespace hammock::renderer {

    /// @struct CompiledSocket
    /// @brief Represents a socket of a task after it had been compiled by GraphCompiler
    export struct CompiledSocket final {
        /// Original socket before compilation (for debug)
        SocketHandle origin;
        /// Handles of physical resources (may be multiple for resources that need to have a copy for each frame in flight)
        std::vector<core::ResourceHandle> handles{};
        /// Binding information (one of DescriptorBinding or AttachmentLocation)
        SocketInterface bindingIface{};

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
    export struct CompiledTask final {
        /// Original task before compilation (for debug)
        TaskHandle origin;
        /// Constructed pipeline
        std::unique_ptr<core::BasePipeline> pipeline{nullptr};

        /// Compiled sockets
        std::vector<CompiledSocket> sockets{};

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
    export struct CompiledDependencyGraph final {
        std::vector<CompiledTask> compiledTasks{};
    };

    /// @class DependencyGraphCompiler
    /// @brief Outputs compiled graph
    export class DependencyGraphCompiler final {
        CompiledDependencyGraph compiledDependencyGraph_{};

    public:

        /// @brief Compiles the dependency. Result can be executed by DependencyGraphExecutor
        CompiledDependencyGraph compileDependencyGraph(DependencyGraph &dependencyGraph) {


            return std::move(compiledDependencyGraph_);
        }
    };
}