#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "core/surface_provider_iface.hpp"
#include "presenter.hpp"
#include "renderer/render_snapshot.hpp"
#include "renderer/renderer.hpp"
#include "utils/spsc_queue.hpp"
#include "utils/thread_safe_queue.hpp"

namespace hammock::app {

    /// @enum RenderCommandType
    /// @brief Describes the type of a render command
    enum class RenderingServerInfoCommandType { Invalid, Ready, Frametime };

    /// @struct RenderCommand
    /// @brief Command data relayed between render queues
    struct RenderingServerInfoCommand {
        RenderingServerInfoCommandType type = RenderingServerInfoCommandType::Invalid;
        uint32_t frameIndex{ 0 };
        float frametime{ 0.f };
        float rendertime{ 0.f };
        float cmdtime{ 0.f };
    };

    /// @struct RenderingServerDrawCommand
    /// @brief Draw data for rendergin server to render
    struct RenderingServerDrawCommand {
        uint64_t frameNumber{ 0 };
        renderer::RenderSnapshotHeader* snapHeader{ nullptr };
    };

    using DrawCommandQueue = ThreadSafeQueue<RenderingServerDrawCommand>;
    using InfoCommandQueue = SPSCQueue<RenderingServerInfoCommand, 16>;

    /// @class RenderProx
    /// relays messages between rendering frontend and rendering backend via thread safe queues
    class RenderingServerProxy {
       private:
        std::atomic<uint64_t> lastCompletedFrame_{ 0xFFFFFFFFFFFFFFFF };
        DrawCommandQueue drawQueue;
        InfoCommandQueue requestQueue;
        InfoCommandQueue responseQueue;

       public:
        DrawCommandQueue& getDrawQueue() { return drawQueue; }
        InfoCommandQueue& getResponseQueue();

        void setLastCompletedFrame(uint64_t frame) {
            lastCompletedFrame_.store(frame, std::memory_order_release);
        }

        uint64_t getLastCompletedFrame() const {
            return lastCompletedFrame_.load(std::memory_order_acquire);
        }
    };

    /// @class RenderingServer
    class RenderingServer {
       public:
        explicit RenderingServer(RenderingServerProxy& proxy, core::SurfaceProviderIface& surfaceProvider,
            std::unique_ptr<renderer::RendererIface>&& renderer, std::unique_ptr<PresenterIface>&& presenter);

        void start();

        void stop();

        void join();

        bool isRunning() const;

       private:
        RenderingServerProxy& proxy_;
        std::atomic<bool> running_{ false };
        std::thread thread_;
        core::SurfaceProviderIface& surfaceProvider_;
        std::unique_ptr<renderer::RendererIface> renderer_{ nullptr };
        std::unique_ptr<PresenterIface> presenter_{ nullptr };
        bool readySent_{ false };
        struct {
            uint32_t width, height;
        } latestSize_;
    };
}  // namespace hammock::app
