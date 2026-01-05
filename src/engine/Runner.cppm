module;

#include <memory>
#include <array>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock_engine.runner;

import hammock_engine.window;
import hammock_renderer.renderer;
import hammock_renderer.graphics_context;
import hammock_engine.ui;
import hammock_engine.presentation_engine;

namespace hammock::engine {

    /// @enum RunnerMode
    /// @brief Defines how the runner launches the engine
    export enum class RunnerMode {
        Headless, /// Running without a rendering surface
        Editor, /// Surface rendering without tooling
        Runtime, /// Both tooling and surface rendering
    };

    /// @class Runner
    /// @brief High-level editor application coordinator
    /// Responsibilities:
    /// - Owns and manages the window
    /// - Coordinates presentation, rendering, and UI systems
    /// - Handles application-level events and lifecycle
    export class Runner {
    public:
        Runner(RunnerMode mode, renderer::GraphicsContext& context);
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
        renderer::GraphicsContext& context_;

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
