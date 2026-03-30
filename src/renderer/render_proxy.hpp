#pragma once
#include "core/swapchain.hpp"
#include "render_types.hpp"
#include "utils/spsc_queue.hpp"

namespace hammock::renderer {
    /// @enum RenderCommandType
    /// @brief Describes the type of a render command
    enum class RenderCommandType {
        Invalid,
        Resize,
        Stop,
        Ready,
        Ui,
        Frametime
    };

    /// @struct RenderCommand
    /// @brief Command data relayed between render queues
    struct RenderCommand {
        RenderCommandType type = RenderCommandType::Invalid;
        uint32_t width{0};
        uint32_t height{0};
        float frametime{0.f};
        float rendertime{0.f};
        float cmdtime{0.f};
    };

    using RenderQueue = threading::SPSCQueue<RenderSnapshot, core::SwapChain::MAX_FRAMES_IN_FLIGHT>;
    using CommandQueue = threading::SPSCQueue<RenderCommand, 16>;

    /// @class RenderProx
    /// relays messages between rendering frontend and rendering backend via thread safe queues
    class RenderProxy {
       private:
        // Queue for render packets
        RenderQueue renderQueue_;
        CommandQueue ftbCommandQueue_;
        CommandQueue btfCommandQueue_;

       public:
        RenderQueue& getRenderQueue();
        CommandQueue& getFtbCommandQueue();
        CommandQueue& getBtfCommandQueue();
    };
}  // namespace hammock::renderer
