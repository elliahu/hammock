#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>

#include "rendering_server.hpp"
#include "dispatcher.hpp"
#include "window.hpp"
#include "frame_arena.hpp"


namespace hammock::app {

    /// @class Runner
    /// @brief High-level application coordinator
    class Runner {
    public:
        Runner();
        ~Runner();

        /// @brief Start the editor main loop
        void run();

    private:
        /// Window
        std::unique_ptr<Window> window_;

        /// Rendering arena
        RenderRingBuffer ringBuffer{};

        /// Render proxy
        std::unique_ptr<app::RenderingServerProxy> renderingServerProxy_{nullptr};

        /// Render frontend
        std::unique_ptr<app::Dispatcher> dispatcher_{nullptr};

        /// Render backend
        std::unique_ptr<app::RenderingServer> renderingServer_{nullptr};
    };
}
