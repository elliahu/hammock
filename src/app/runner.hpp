#pragma once
#include <memory>
#include <array>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "core/vulkan_context.hpp"
#include "window.hpp"
#include "ui.hpp"
#include "presentation_engine.hpp"
#include "renderer/renderer.hpp"

namespace hammock::app {

    /// @enum RunnerMode
    /// @brief Defines how the runner launches the engine
    enum class RunnerMode {
        Tooling, 
        Game, 
    };

    /// @class Runner
    /// @brief High-level editor application coordinator
    /// Responsibilities:
    /// - Owns and manages the window
    /// - Coordinates presentation, rendering, and UI systems
    /// - Handles application-level events and lifecycle
    class Runner {
    public:
        Runner(RunnerMode mode, core::VulkanContext& vulkanContext);
        ~Runner();

        /// @brief Start the editor main loop
        void launch();

        /// @brief Check if editor should exit
        bool shouldClose() const { return window_->shouldClose(); }

    private:
        void loop();
        void handleInput();
        void update(float deltaTime);

        // Core Systems (in order of ownership dependency)

        /// Graphics context (reference, owned by Application)
        core::VulkanContext& vulkanContext_;

        /// Window (owned by Editor)
        std::unique_ptr<Window> window_;

        /// Presentation system (owns framebuffer, swapchain, sync)
        std::unique_ptr<PresentationEngine> presentationEngine_;

        /// Renderer (uses presentation engine for targets)
        std::unique_ptr<renderer::Renderer> renderer_;

        /// UI system (editor mode only)
        std::unique_ptr<Ui> ui_;
    };
}
