module;

#include <memory>

export module hammock.engine.editor;

import :window;
import hammock.renderer.renderer;
import hammock.renderer.graphics_context;


namespace hammock::engine {
    /// @enum EngineMode
    /// @brief Describes how the engine is launched
    export enum class EngineMode {
        Editor,
        Runtime,
    };

    /// @class Editor
    /// @brief Represents engines editor
    export class Editor final {
    public:
        Editor(const EngineMode mode) : mode(mode) {
            context = std::make_unique<renderer::GraphicsContext>(renderer::GraphicsContextDesc{
                .mode = renderer::GraphicsContextMode::Windowed,
            });
            window = std::make_unique<Window>("Hammock engine", context->getInstance(), 1920u, 1080u);
            context->attachSurface(window->getSurface());
            auto strategy = std::make_unique<renderer::DeferredRenderingStrategy>(*context);
            renderer = std::make_unique<renderer::Renderer>(*context, std::move(strategy));
        }

        ~Editor() {
        }

        void launch() const {
            loop();
        }

    private:
        void loop() const {
            while (!window->shouldClose()) {
                window->pollEvents();
            }
        }

        EngineMode mode;
        std::unique_ptr<renderer::GraphicsContext> context;
        std::unique_ptr<renderer::Renderer> renderer;
        std::unique_ptr<engine::Window> window;
    };
}
