module;

#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <variant>
#include <vulkan/vulkan.hpp>

export module hammock_renderer:socket;

import hammock_core;

namespace hammock::renderer {
    /// @struct LogicalResourceHandle
    export struct LogicalResourceHandle {
        std::uint32_t index;
        std::uint32_t generation;
    };

    /// @enum ImageType
    /// Describes the type of the image in the socket
    export enum class ImageType {
        Undefined,
        Type2D,
        Type3D,
    };

    export enum class ImageFormat {
        Undefined,
        R8G8B8A8Uint,
        R16G16B16A16Sfloat,
        R32G32B32A32Sfloat,
    };

    export struct ImageLogicalResource {
        ImageFormat format = ImageFormat::Undefined;
        ImageType type = ImageType::Type2D;
    };

    export struct BufferLogicalResource {
    };

    export using LogicalResourceInterface = std::variant<
        std::monostate,
        ImageLogicalResource,
        BufferLogicalResource
    >;

    /// @struct SocketHandle
    /// @brief Returned when adding a task to TaskGraph.
    /// Represents a task inside a TaskGraph
    export struct SocketHandle {
        std::uint32_t index;
        std::uint32_t generation;
    };

    /// @enum SocketUsageStageFlagBits
    /// @brief Describes the stage at which the socket is used
    export enum SocketUsageStageFlagBits {
        Unused = 0,
        ComputeShader = 1 << 0,
        VertexShader = 1 << 1,
        FragmentShader = 1 << 2,
    };


    /// @struct DescriptorBinding
    /// @brief Describes descriptor binding info (if the socket is descriptor)
    export struct DescriptorBinding {
        std::uint32_t set;
        std::uint32_t binding;
        std::int32_t usage;
        std::uint32_t count = 1u;
    };

    /// @struct AttachmentLocation
    /// @brief Describes attachment location info (if the socket is attachment)
    export struct AttachmentLocation {
        std::uint32_t location = 0u; // color attachment index or depth/stencil slot
    };

    /// @typedef SocketInterface
    /// @brief Describes what kind of socket we have
    export using SocketInterface = std::variant<
        std::monostate, // no binding info
        DescriptorBinding, // descriptor-backed
        AttachmentLocation // render pass attachment
    >;


    /// @interface BaseSocket
    /// Base interface for resource sockets.
    class BaseSocket {
        friend class DependencyGraphCompiler;
        std::string name_;
        SocketInterface iface_;
        LogicalResourceHandle handle_;

    protected:
        // Protected constructor so that only children can instantiate
        explicit BaseSocket(std::string name, LogicalResourceHandle handle,
                            SocketInterface iface = {}) : name_(name), handle_(handle), iface_(iface) {
        };

    public:
        virtual ~BaseSocket() = default;

        [[nodiscard]] std::string getName() const { return name_; }
    };


    /// @enum ImageUsage
    /// @breif Describes how the image in the socket is used by the task
    export enum class ImageUsage {
        Undefined, // Invalid/Initial state
        ColorAttachmentWrite, // Rendering target color
        DepthAttachmentWrite, // Rendering target depth (stencil)
        SampledRead, // Used as combined image sampler
        StorageReadWrite, // Storage image (read write access implied)
        TransferSrc, // Transfer source (data will be copied into the image)
        TransferDst, // Transfer destination (data will be copied out of the image)
        Present // Image will be used as present image
    };

    /// @class ImageSocket
    /// Concrete Socket for image
    export class ImageSocket final : public BaseSocket {
        friend class DependencyGraphCompiler;
        ImageUsage usage_ = ImageUsage::Undefined;

    public:
        ImageSocket(std::string name, LogicalResourceHandle handle, const ImageUsage usage,
                    SocketInterface iface = {}) : BaseSocket(name, handle, iface), usage_(usage) {
        }
    };

    /// @enum BufferUsage
    /// @brief Describes how is buffer accessed
    export enum class BufferUsage {
        Undefined,
        UniformBufferRead,
        StorageBufferReadWrite,
    };

    /// @class BufferSocket
    /// @brief Concrete socket for buffer
    export class BufferSocket final : public BaseSocket {
        friend class DependencyGraphCompiler;
        BufferUsage usage_;

    public:
        BufferSocket(std::string name, LogicalResourceHandle handle, const BufferUsage usage,
                     DescriptorBinding binding) : BaseSocket(name, handle, binding), usage_(usage) {
        }
    };
}
