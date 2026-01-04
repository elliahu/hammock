module;

#include <memory>
#include <array>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock.engine.runner;

import hammock.engine.window;
import hammock.renderer.renderer;
import hammock.renderer.graphics_context;
import hammock.engine.ui;
import hammock.engine.presentation_engine;

namespace hammock::engine {

    export enum class RunnerMode {
        Headless,
        Editor,
        Runtime,
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
