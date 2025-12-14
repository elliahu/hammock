module;

#include <string>
#include <utility>

export module hammock.renderer.task_graph:socket;

import hammock.core.base_resource;

namespace hammock::renderer {
    class ISocket {
        core::ResourceHandle handle;
        std::string name;

    public:
        virtual ~ISocket() = default;

        explicit ISocket(std::string name, const core::ResourceHandle handle) : handle(handle), name(std::move(name)) {
        }
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
        ImageSocket(const std::string &name, const core::ResourceHandle handle, const ImageState state) : ISocket(name, handle), state(state) {
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
        BufferSocket(const std::string &name, const core::ResourceHandle handle, const BufferState state) : ISocket(name, handle), state(state) {
        }
    };
}
