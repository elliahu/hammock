#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "base_resource.hpp"
#include "command_buffer.hpp"
#include "core/utilities.hpp"
#include "device.hpp"

namespace hammock::graph {

    // ********** Bse GPU task **********

    /// @struct LogicalResourceHandle
    struct LogicalResourceHandle {
        std::uint32_t index;
        std::uint32_t generation;

        bool operator==(const LogicalResourceHandle& other) const {
            return generation == other.generation && index == other.index;
        }
    };

    struct LogicalResourceHandleHash {
        std::size_t operator()(const LogicalResourceHandle& h) const noexcept {
            return (static_cast<std::size_t>(h.generation) << 32) | static_cast<std::size_t>(h.index);
        }
    };


    /// @enum ImageAccess
    /// @breif Describes how the image in the socket is used by the task
    enum class ImageAccess {
        Undefined,             // Invalid/Initial state
        ColorAttachmentWrite,  // Rendering target color
        DepthAttachmentWrite,  // Rendering target depth (stencil)
        SampledRead,           // Used as combined image sampler
        StorageReadWrite,      // Storage image (read write access implied)
    };

    /// @enum BufferAccess
    /// @brief Describes how is buffer accessed
    enum class BufferAccess {
        Undefined,
        UniformRead,
        StorageReadWrite,
    };

    /// @typedef AccessInterface
    /// @brief Defines what kind of resource is accessed and in what manner. One of ImageAccess or
    /// BufferAccess
    using AccessInterface = std::variant<ImageAccess, BufferAccess>;

    /// @struct LogicalResourceAccess
    /// @brief Defines a single resource access in the task
    struct LogicalResourceAccess {
        LogicalResourceHandle handle;
        AccessInterface access;
    };

    /// @struct TaskHandle
    /// @brief Returned when adding a task to TaskGraph.
    /// Represents a task inside a TaskGraph
    struct TaskHandle {
        std::uint32_t index;
        std::uint32_t generation;

        bool operator==(const TaskHandle& other) const {
            return generation == other.generation && index == other.index;
        }
    };

    /// @typedef GpuTaskContextImageResolverFunction
    using GpuTaskContextImageResolverFunction =
        std::function<core::Handle<core::Image>(LogicalResourceHandle, uint32_t)>;

        /// @typedef GpuTaskContextBufferResolverFunction
    using GpuTaskContextBufferResolverFunction =
        std::function<core::Handle<core::Buffer>(LogicalResourceHandle, uint32_t)>;

    /// @class GpuTaskContext
    /// @brief Represents a context given to each GPU task when it is executed
    /// Can be used to access resources in the execution function and perform custom logic
    class GpuTaskExecContext final {
        friend class DependencyGraphCompiler;

       public:
        explicit GpuTaskExecContext(core::CommandBuffer& cmd, uint32_t frameIdx)
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
        GpuTaskContextImageResolverFunction imageResolver_{nullptr};
        GpuTaskContextBufferResolverFunction bufferResolver_{nullptr};
        core::CommandBuffer& cmd_;
        uint32_t frameIdx_;
    };

    /// @class GpuTaskCompileContext
    /// @brief Represents a context given to a task during a compilation.
    /// Use this to create you pipelines / descriptors etc.
    class GpuTaskCompileContext final {
        friend class DependencyGraphCompiler;

       public:
        // TODO
    };

    /// @class GpuTaskDeclBuilder
    /// @brief Build that is used to declare resource accesses
    class GpuTaskDeclBuilder final {
        std::vector<LogicalResourceAccess> logicalResourceAccesses_{};

       public:
        void access(LogicalResourceAccess access);
    };

    /// @typedef TaskExecutionFunction
    /// @brief A function that gets executed when task is run
    /// Do your rendering here
    using TaskExecutionFunction = std::function<void(GpuTaskExecContext& ctx)>;

    /// @typedef TaskCompilationFunction
    /// @brief A function that gets executed when task is being compiled
    /// Do your pipeline/descriptor setup here
    using TaskCompilationFunction = std::function<void(GpuTaskCompileContext& ctx)>;

    /// @typedef TaskDeclarationFunction
    /// @brief A function that gets executed when graph is declared
    /// Do your resource declaration here
    using TaskDeclarationFunction = std::function<void(GpuTaskDeclBuilder& builder)>;

    /// @interface BaseGpuTask
    /// Interface representing general GPU task that has input and outputs (sockets)
    class GpuTask final {
        friend class DependencyGraphCompiler;
        friend class GpuTaskDeclBuilder;

       public:
        explicit GpuTask(core::CommandQueueFamily family = core::CommandQueueFamily::Ignored)
            : family_(family) {}
        ~GpuTask() = default;

        /// @brief Returns a command queue family of the task
        core::CommandQueueFamily getFamily() { return family_; }

        /// @brief Declare resource access for this task
        /// @note It is recommended that you use decl callback for this as this method might get removed in
        /// future versions.
        void access(LogicalResourceAccess access);

        /// @brief Declaration time callback
        /// here you declare the tasks resource accesses
        void decl(TaskDeclarationFunction func) { declFunc_ = std::move(func); }

        /// @brief Compilation time callback
        /// here you create your pipelines and descriptors and do your initialization
        void compile(TaskCompilationFunction func) { compFunc_ = std::move(func); }

        /// @brief Execution time callback
        /// here you do your rendering / dispatch / transfer
        void exec(TaskExecutionFunction func) { execFunc_ = std::move(func); }

       protected:
        core::CommandQueueFamily family_ =
            core::CommandQueueFamily::Graphics;  // What type of the task we have
        std::vector<LogicalResourceAccess> logicalResourceAccesses_{};
        TaskDeclarationFunction declFunc_{nullptr};  // Declaration function
        TaskCompilationFunction compFunc_{nullptr};  // Compilation function
        TaskExecutionFunction execFunc_{nullptr};    // Execution function
    };

}  // namespace hammock::graph
