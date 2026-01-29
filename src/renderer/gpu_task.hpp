#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "push_constants.hpp"
#include "hammock_core.hpp"


namespace hammock::renderer {

    // ********** Bse GPU task **********

    /// @struct LogicalResourceHandle
    struct LogicalResourceHandle {
        std::uint32_t index;
        std::uint32_t generation;

        bool operator==(const LogicalResourceHandle& other) const {
            return generation == other.generation && index == other.index;
        }
    };

    // Hash function
    struct LogicalResourceHandleHash {
        std::size_t operator()(const LogicalResourceHandle& handle) const noexcept {
            std::size_t h1 = std::hash<std::uint32_t>{}(handle.index);
            std::size_t h2 = std::hash<std::uint32_t>{}(handle.generation);
            return h1 ^ (h2 << 1);  // simple hash combine
        }
    };

    /// @enum SocketUsageStageFlagBits
    /// @brief Describes the stage at which the socket is used
    enum SocketUsageStageFlagBits {
        Unused = 0,
        ComputeShader = 1 << 0,
        VertexShader = 1 << 1,
        FragmentShader = 1 << 2,
    };

    /// @struct DescriptorBinding
    /// @brief Describes descriptor binding info (if the socket is descriptor)
    struct DescriptorBinding {
        std::uint32_t set;
        std::uint32_t binding;
        std::int32_t usage;
        std::uint32_t count = 1u;
    };

    /// @struct AttachmentLocation
    /// @brief Describes attachment location info (if the socket is attachment)
    struct AttachmentLocation {
        std::uint32_t location = 0u;  // color attachment index or depth/stencil slot
    };

    /// @typedef BindingInterface
    /// @brief Defines how exactly is the resource bound in the task. One of DescriptorBinding or
    /// AttachmentLocation.
    using BindingInterface = std::variant<std::monostate, DescriptorBinding, AttachmentLocation>;

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
        UniformBufferRead,
        StorageBufferReadWrite,
    };

    /// @typedef AccessInterface
    /// @brief Defines what kind of resource is accessed and in what manner. One of ImageAccess or
    /// BufferAccess
    using AccessInterface = std::variant<std::monostate, ImageAccess, BufferAccess>;

    /// @struct LogicalResourceAccess
    /// @brief Defines a single resource access in the task
    struct LogicalResourceAccess {
        LogicalResourceHandle handle;
        AccessInterface access;
        BindingInterface binding;
    };

    /// @struct TaskHandle
    /// @brief Returned when adding a task to TaskGraph.
    /// Represents a task inside a TaskGraph
    struct TaskHandle {
        std::uint32_t index;
        std::uint32_t generation;
    };

    /// @interface BaseGpuTask
    /// Interface representing general GPU task that has input and outputs (sockets)
    class BaseGpuTask : public core::DynamicCastHelper {
        friend class DependencyGraphCompiler;

       public:
        virtual ~BaseGpuTask() = default;

        /// @brief Add push constant block to the task
        /// @returns reference to the added push constant block
        PushConstantsBlock* addPushConstantBlock(std::unique_ptr<PushConstantsBlock>&& block);

        /// @brief Declare resource access for this task
        void access(LogicalResourceAccess access);

       protected:
        std::unique_ptr<PushConstantsBlock> pushConstantsBlock_{
            nullptr};  // Single push constant block allowed
        std::unordered_map<LogicalResourceHandle, std::vector<LogicalResourceAccess>, LogicalResourceHandleHash>
            logicalResourceAccesses_;
    };

    // ************ Graphics Task *************

    /// @class GraphicsTask
    /// @brief Specialized task that uses standard raster pipline.
    /// Vertex and fragment shaders required
    /// TODO use reflection to build the task from SPIR-V shader
    class GraphicsTask final : public BaseGpuTask {
        friend class DependencyGraphCompiler;

       public:
        GraphicsTask(const std::string& vertexShaderFile, const std::string& fragmentShaderFile)
            : vertexShaderFile_(vertexShaderFile), fragmentShaderFile_(fragmentShaderFile) {}

       private:
        std::string vertexShaderFile_;
        std::string fragmentShaderFile_;
    };

    // *********** Compute Task ***********

    /// @class ComputeTask
    /// @brief Specialized task that uses GPU compute via compute shader
    /// /// TODO use reflection to build the task from SPIR-V shader
    class ComputeTask final : public BaseGpuTask {
        friend class DependencyGraphCompiler;

       public:
        explicit ComputeTask(const std::string& computeShaderFile) : computeShaderFile_(computeShaderFile) {}

       private:
        std::string computeShaderFile_;
    };
}  // namespace hammock::renderer
