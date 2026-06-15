#pragma once
#include <cstdint>
#include <thread>
#include "rendering_server.hpp"
#include "renderer/render_snapshot.hpp"
#include "core/surface_provider_iface.hpp"
#include "frame_arena.hpp"

namespace hammock::app {

    class Dispatcher {
       public:
        explicit Dispatcher(RenderingServerProxy& proxy, core::SurfaceProviderIface& surfaceProvider, RenderRingBuffer& ringBuffer);

        /// @brief Start the execution
        void start();

       private:
        void waitForBackend();
        // Handles os inputs here
        void handleInput() {};
        // Updates the state
        void update();
        // Builds the render snapshot
        renderer::RenderSnapshotHeader buildRenderSnapshot();

        RenderingServerProxy& renderingServerProxy_;
        core::SurfaceProviderIface& surfaceProvider_;
        RenderRingBuffer& ringBuffer_;
        float frameTime_{0.f};
        float renderTime_{0.f};
        float cmdTime_{0.f};
        uint64_t frameNumber_{0};
    };
}  // namespace hammock::renderer
