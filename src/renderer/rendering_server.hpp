#pragma once

#include <memory>
#include <thread>

#include "presenter.hpp"
#include "renderer.hpp"
#include "surface_provider_iface.hpp"
#include "utils/spsc_queue.hpp"

namespace hammock::renderer {

    /// @enum RenderCommandType
    /// @brief Describes the type of a render command
    enum class RenderCommandType { Invalid, Resize, Stop, Ready, Ui, Frametime };

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
    class RenderingServerProxy {
       private:
        // Queue for render packets
        RenderQueue renderQueue_;
        CommandQueue requestQueue;
        CommandQueue responseQueue;

       public:
        RenderQueue& getRenderQueue();
        CommandQueue& getFtbCommandQueue();
        CommandQueue& getBtfCommandQueue();
    };

    class RenderingServer {
       public:
        explicit RenderingServer(RenderingServerProxy& proxy, core::SurfaceProviderIface& surfaceProvider,
            std::unique_ptr<RendererIface>&& renderer, std::unique_ptr<PresenterIface>&& presenter);

        void start();

        void stop();

        void join();

        bool isRunning() const;

       private:
        RenderingServerProxy& proxy_;
        std::atomic<bool> running_{false};
        std::thread thread_;
        core::SurfaceProviderIface& surfaceProvider_;
        std::unique_ptr<RendererIface> renderer_{nullptr};
        std::unique_ptr<PresenterIface> presenter_{nullptr};
        bool readySent_{false};
    };
}  // namespace hammock::renderer
