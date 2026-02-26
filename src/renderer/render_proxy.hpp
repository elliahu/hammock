#pragma once
#include "core/swapchain.hpp"
#include "render_types.hpp"
#include "ui.hpp"
#include "utils/spsc_queue.hpp"

namespace hammock::renderer {
    /// @enum RenderCommandType
    /// @brief Describes the type of a render command
    enum class RenderCommandType {
        None,
        Resize,
        Stop,
        Ready,
        Ui,
    };

    /// @struct RenderCommand
    /// @brief Command data relayed between render queues
    struct RenderCommand {
        RenderCommandType type = RenderCommandType::None;
        uint32_t width{0};
        uint32_t height{0};
    };

    using RenderQueue = threading::SPSCQueue<RenderSnapshot, core::SwapChain::MAX_FRAMES_IN_FLIGHT>;
    using CommandQueue = threading::SPSCQueue<RenderCommand, 16>;
    using UserInterfaceQueue = threading::SPSCQueue<UiSnapshot, core::SwapChain::MAX_FRAMES_IN_FLIGHT>;

    /// @class RenderProx
    /// relays messages between rendering frontend and rendering backend via thread safe queues
    class RenderProxy {
       private:
        // Queue for render packets
        RenderQueue renderQueue_;
        CommandQueue ftbCommandQueue_;
        CommandQueue btfCommandQueue_;
        UserInterfaceQueue userInterfaceQueue_;

       public:
        RenderQueue& getRenderQueue();
        CommandQueue& getFtbCommandQueue();
        CommandQueue& getBtfCommandQueue();
        UserInterfaceQueue& getUserInterfaceQueue() { return userInterfaceQueue_; }
    };
}  // namespace hammock::renderer