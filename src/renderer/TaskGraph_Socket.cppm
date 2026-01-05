module;

#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <variant>

export module hammock_renderer:socket;

import hammock_core;

namespace hammock::renderer {
    /// @enum SocketUsageStage
    /// @brief Describes the stage at which the socket is used
    export enum SocketUsageStage {
        None = 0,
        ComputeShader = 1 << 0,
        VertexShader = 1 << 1,
        FragmentShader = 1 << 2,
    };

    /// @struct DescriptorBinding
    /// @brief Describes descriptor binding
    export struct DescriptorBinding {
        std::uint32_t set;
        std::uint32_t binding;
        SocketUsageStage usage;
        std::uint32_t count = 1u;
    };

    /// @struct AttachmentLocation
    /// @brief Describes attachment location
    export struct AttachmentLocation {
        std::uint32_t location; // color attachment index or depth/stencil slot
    };

    /// @typedef SocketInterface
    /// @brief Describes what kind of socket we have
    export using SocketInterface = std::variant<
        std::monostate, // no binding info
        DescriptorBinding, // descriptor-backed
        AttachmentLocation // render pass attachment
    >;


    /// @interface ISocket
    /// Base interface for resource sockets.
    class ISocket {
        std::vector<core::ResourceHandle> handles;
        std::string name;
        SocketInterface iface;

    public:
        virtual ~ISocket() = default;

        explicit ISocket(const std::string &name, SocketInterface iface = {}) : name(name), iface(iface) {
            if (auto binding = get_if<DescriptorBinding>(&iface)) {
                handles.resize(binding->count);
            }
        }

        /// @brief Assign resource to specific index in the socket. If socket is not array, leave index = 0
        void assignResource(core::ResourceHandle resource, std::uint32_t index = 0) {
            if (index >= handles.size()) {
                throw std::out_of_range("invalid index");
            }
            handles[index] = resource;
        }

        [[nodiscard]] std::string getName() const { return name; }
    };


    /// @enum ImageState
    /// @breif Describes how the image in the socket is used by the task
    export enum class ImageState {
        Undefined, // Invalid/Initial state
        ColorAttachmentWrite, // Rendering target color
        DepthAttachmentWrite, // Rendering target depth (stencil)
        SampledRead, // Used as combined image sampler
        StorageReadWrite, // Storage image (read write access implied)
        TransferSrc, // Transfer source (data will be copied into the image)
        TransferDst, // Transfer destination (data will be copied out of the image)
        Present // Image will be used as present image
    };

    /// @enum ImageType
    /// Describes the type of the image in the socket
    export enum class ImageType {
        Undefined,
        Type2D,
        Type3D,
    };

    /// @class ImageSocket
    /// Concrete Socket for image
    export class ImageSocket final : public ISocket {
        ImageState state = ImageState::Undefined;
        ImageType type = ImageType::Undefined;

    public:
        ImageSocket(std::string name, const ImageState state, const ImageType type,
                    SocketInterface iface = {}) : ISocket(name, iface),
                                                  state(state), type(type) {
        }
    };

    /// @enum BufferState
    /// @brief Describes how is buffer accessed
    export enum class BufferState {
        Undefined,
        UniformBufferRead,
        StorageBufferReadWrite,
    };

    /// @class BufferSocket
    /// @brief Concrete socket for buffer
    export class BufferSocket final : public ISocket {
        BufferState state;

    public:
        BufferSocket(std::string name, const BufferState state, DescriptorBinding binding) : ISocket(name, binding),
            state(state) {
        }
    };
}
