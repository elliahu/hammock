export module hammock.renderer.task_graph:socket;

import hammock.core.base_resource;

namespace hammock::renderer {
    class ISocket {
        core::ResourceHandle handle;

    public:
        virtual ~ISocket() = default;

        explicit ISocket(const core::ResourceHandle handle) : handle(handle) {
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

    class ImageSocket final : public ISocket {
        ImageState state;

    public:
        ImageSocket(const core::ResourceHandle handle, const ImageState state) : ISocket(handle), state(state) {
        }
    };

    enum class BufferState {
        Undefined,
        UniformBuffer,
        StorageBuffer,
    };

    class BufferSocket final : public ISocket {
        BufferState state;

    public:
        BufferSocket(const core::ResourceHandle handle, const BufferState state) : ISocket(handle), state(state) {
        }
    };
}
