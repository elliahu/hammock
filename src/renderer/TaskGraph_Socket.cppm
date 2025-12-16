module;

#include <string>
#include <utility>
#include <memory>
#include <stdexcept>
#include <cstdint>

export module hammock.renderer.task_graph:socket;

import hammock.core.base_resource;
import hammock.renderer.spirv_reflection;

namespace hammock::renderer {

    /// @enum SocketUsageStage
    /// Describes the stage at which the
    enum SocketUsageStage {
        None = 0,
        ComputeShader = 1 << 0,
        VertexShader = 1 << 1,
        FragmentShader = 1 << 2,
    };

    /// @class ISocket
    /// Base interface for task sockets.
    class ISocket {
        core::ResourceHandle handle;
        std::string name;
        int usage = SocketUsageStage::None;

    public:
        virtual ~ISocket() = default;

        explicit ISocket(std::string name) : name(std::move(name)) {
        }

        void assignResource(core::ResourceHandle resource) { handle = resource; }
        void addUsage(SocketUsageStage flag) { usage |= flag; }
        [[nodiscard]] std::string getName() const { return name; }
    };

    enum class ImageState {
        Undefined, // Invalid/Initial state
        ColorAttachmentWrite, // Rendering target color
        DepthAttachmentWrite, // Rendering target depth (stencil)
        SampledRead, // Used as combined image sampler
        StorageReadWrite, // Storage image (read write access implied)
        TransferSrc, // Transfer source (data will be copied into the image)
        TransferDst, // Transfer destination (data will be copied out of the image)
        Present // Image will be used as present image
    };

    enum class ImageType {
        Undefined,
        Type2D,
        Type3D,
    };


    /// Concrete Socket for image
    class ImageSocket final : public ISocket {
        ImageState state = ImageState::Undefined;
        ImageType type = ImageType::Undefined;

    public:
        ImageSocket(const std::string &name, const ImageState state, const ImageType type) : ISocket(name),
            state(state), type(type) {
        }

        /// Create socket from descriptor binding
        ImageSocket(const reflection::SpvReflectDescriptorBinding *binding) : ISocket(binding->name) {
            switch (binding->image.dim) {
                case reflection::SpvDim::SpvDim2D:
                    type = ImageType::Type2D;
                    break;
                case reflection::SpvDim::SpvDim3D:
                    type = ImageType::Type3D;
                    break;
                default:
                    throw std::invalid_argument("Unsupported image type");
            }

            switch (binding->descriptor_type) {
                case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    state = ImageState::SampledRead;
                    break;
                case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    state = ImageState::StorageReadWrite;
                    break;
                default:
                    throw std::invalid_argument("Unsupported image state");
            }
        }
    };

    enum class BufferState {
        Undefined,
        UniformBufferRead,
        StorageBufferReadWrite,
    };

    /// Concrete socket for buffer
    class BufferSocket final : public ISocket {
        BufferState state;

    public:
        BufferSocket(const std::string &name, const BufferState state) : ISocket(name), state(state) {
        }

        /// Create socket from descriptor binding
        BufferSocket(const reflection::SpvReflectDescriptorBinding *binding) : ISocket(binding->name) {
        }
    };
}
