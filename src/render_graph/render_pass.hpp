#pragma once
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "command_buffer.hpp"
#include "device.hpp"
#include "render_graph_types.hpp"

namespace hammock::graph {

    /// @typedef RenderPassExecContextImageResolverFunction
    using RenderPassExecContextImageResolverFunction =
        std::function<core::Handle<core::Image>(LogicalResourceHandle, uint32_t)>;

    /// @typedef RenderPassExecContextBufferResolverFunction
    using RenderPassExecContextBufferResolverFunction =
        std::function<core::Handle<core::Buffer>(LogicalResourceHandle, uint32_t)>;

    /// @class RenderPassExecContext
    /// @brief Represents a context given to each GPU task when it is executed
    /// Can be used to access resources in the execution function and perform custom logic
    class RenderPassExecContext final {
        friend class DependencyGraphCompiler;

       public:
        explicit RenderPassExecContext(core::CommandBuffer& cmd, uint32_t frameIdx)
            : cmd_(cmd), frameIdx_(frameIdx) {}

        /// @brief Returns the index of the current frame
        /// Use this to select a resource for the frame in flight
        uint32_t getFrameIndex() { return frameIdx_; }

        /// @brief Returns the command buffer for this gpu task
        core::CommandBuffer& getCommandBuffer() { return cmd_; }

        core::Handle<core::Image> resolveImage(LogicalResourceHandle handle);
        core::Handle<core::Buffer> resolveBuffer(LogicalResourceHandle handle);

       private:
        /// This function is used to resolve the actual physical resource handle created by the resource
        /// manager from the logical resource handle creates by high level graph
        RenderPassExecContextImageResolverFunction imageResolver_{nullptr};
        RenderPassExecContextBufferResolverFunction bufferResolver_{nullptr};
        core::CommandBuffer& cmd_;
        uint32_t frameIdx_;
    };

    /// @class RenderPassComplContext
    /// @brief Represents a context given to a task during a compilation.
    /// Use this to create you pipelines / descriptors etc.
    class RenderPassComplContext final {
        friend class RenderGraphCompiler;

       public:
        // TODO
    };

    /// @class RenderPassBuilder
    /// @brief Build that is used to declare resource accesses
    class RenderPassBuilder final {
        friend class RenderGraphCompiler;
        std::vector<LogicalResourceAccess> logicalResourceAccesses_{};

       public:
        void access(LogicalResourceAccess access);
    };

    /// @typedef RenderPassExecFunc
    /// @brief A function that gets executed when task is run
    using RenderPassExecFunc = std::function<void(RenderPassExecContext& ctx)>;

    /// @typedef RenderPassComplFunc
    /// @brief A function that gets executed when task is being compiled
    using RenderPassComplFunc = std::function<void(RenderPassComplContext& ctx)>;

    /// @typedef RenderPassDeclFunc
    /// @brief A function that gets executed when graph is declared
    using RenderPassBuildFunc = std::function<void(RenderPassBuilder& builder)>;

    /// @class RenderPass
    /// @brief Represents a GPU task that has input and outputs (sockets)
    class RenderPass final {
        friend class RenderGraphCompiler;
        friend class RenderPassBuilder;

       public:
        explicit RenderPass(core::CommandQueueFamily family = core::CommandQueueFamily::Ignored)
            : family_(family) {}
        ~RenderPass() = default;

        /// @brief Returns a command queue family of the task
        core::CommandQueueFamily getFamily() { return family_; }

        /// @brief Declaration time callback
        /// here you declare the tasks resource accesses
        void build(RenderPassBuildFunc func) { declFunc_ = std::move(func); }

        /// @brief Compilation time callback
        /// here you create your pipelines and descriptors and do your initialization
        void compile(RenderPassComplFunc func) { compFunc_ = std::move(func); }

        /// @brief Execution time callback
        /// here you do your rendering / dispatch / transfer
        void exec(RenderPassExecFunc func) { execFunc_ = std::move(func); }

       protected:
        core::CommandQueueFamily family_ =
            core::CommandQueueFamily::Graphics;  // What type of the task we have
        std::vector<LogicalResourceAccess> logicalResourceAccesses_{};
        RenderPassBuildFunc declFunc_{nullptr};  // Declaration function
        RenderPassComplFunc compFunc_{nullptr};  // Compilation function
        RenderPassExecFunc execFunc_{nullptr};   // Execution function
    };

}  // namespace hammock::graph
