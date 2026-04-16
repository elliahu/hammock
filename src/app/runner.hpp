#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>

#include "rendering_server.hpp"
#include "render_frontend.hpp"
#include "window.hpp"


namespace hammock::app {

    /// @class Runner
    /// @brief High-level application coordinator
    class Runner {
    public:
        Runner();
        ~Runner();

        /// @brief Start the editor main loop
        void launch();

    private:
        /// Window
        std::unique_ptr<Window> window_;

        /// Render proxy
        std::unique_ptr<renderer::RenderingServerProxy> renderingServerProxy_{nullptr};

        /// Render frontend
        std::unique_ptr<renderer::RenderFrontend> renderFrontend_{nullptr};

        /// Render backend
        std::unique_ptr<renderer::RenderingServer> renderingServer_{nullptr};
    };
}
