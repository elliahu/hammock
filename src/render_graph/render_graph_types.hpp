#pragma once

#include <cstdint>
#include <variant>
#include <functional>

#include "core/buffer.hpp"
#include "core/image.hpp"
#include "core/resource_manager.hpp"

namespace hammock::graph {
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

    /// @typedef ImportedResourceResolver
    /// Function signature for resource resolver callback
    using ImportedImageResolver = std::function<core::Handle<core::Image>(uint32_t)>;
    using ImportedBufferResolver = std::function<core::Handle<core::Image>(uint32_t)>;

    /// @struct ImportedImageResource
    struct ImportedImageResource{
        ImportedImageResolver resolver{nullptr};
    };

    /// @struct ImportedBufferResource
    struct ImportedBufferResource{
        ImportedBufferResolver resolver{nullptr};
    };

    /// @struct LogicalImageResource
    /// Describes logical image resource
    struct LogicalImageResource {
        core::ImageFormat format = core::ImageFormat::Undefined;
        core::ImageType type = core::ImageType::Type2D;
        core::ImageUsage usage;  // TODO this can be inferred
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t channels = 4;
        std::uint32_t depth = 1;
        std::uint32_t levels = 1;
        std::uint32_t layers = 1;
        bool frameLocal = true;
        bool persistent = true;
    };

    /// @struct LogicalBufferResource
    /// Describes logical buffer resource
    struct LogicalBufferResource {
        core::BufferType type;
        core::BufferUsage usage;
        std::uint64_t instanceSize;
        std::uint32_t instanceCount;
        bool frameLocal = true;
        bool persistent = true;
    };

    /// @typedef LogicalResourceInterface
    /// Describes an option between LogicalImageResource and LogicalBufferResource
    using LogicalResourceInterface = std::variant<LogicalImageResource, LogicalBufferResource, ImportedImageResource, ImportedBufferResource>;

    /// @enum DependencyType
    /// @brief Describes a type of dependency between two tasks
    enum class DependencyType {
        Execution,  /// Task B must not begin until Task A has completed
        Debug,      /// Asserts during compilation that A -> B exists, if not, throws
    };

    /// @struct RenderPassHandle
    /// @brief Returned when adding a task to TaskGraph.
    /// Represents a task inside a TaskGraph
    struct RenderPassHandle {
        std::uint32_t index;
        std::uint32_t generation;

        bool operator==(const RenderPassHandle& other) const {
            return generation == other.generation && index == other.index;
        }
    };
}  // namespace hammock::graph
