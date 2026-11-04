#pragma once
#include <filesystem>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "dispatcher.hpp"
#include "frame_arena.hpp"
#include "launch_config.hpp"
#include "rendering_server.hpp"
#include "window.hpp"

namespace hammock::app {

    /// @class Runner
    /// @brief High-level application coordinator
    class Runner {
       public:
        Runner(const std::filesystem::path& path);
        ~Runner();

        /// @brief Start the editor main loop
        void run();

       private:
        void setWindowCallbacks();

        /// Config
        LaunchConfig config_;

        /// Window
        std::unique_ptr<Window> window_;

        /// Rendering arena
        RenderRingBuffer ringBuffer{};

        /// Render proxy
        std::unique_ptr<app::RenderingServerProxy> renderingServerProxy_{ nullptr };

        /// Render frontend
        std::unique_ptr<app::Dispatcher> dispatcher_{ nullptr };

        /// Render backend
        std::unique_ptr<app::RenderingServer> renderingServer_{ nullptr };
    };
}  // namespace hammock::app
