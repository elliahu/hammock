module;

#include <memory>

export module hammock.engine.editor;

import :window;
import hammock.renderer.renderer;


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
        Editor(const EngineMode mode) : mode(mode),
                                        renderer(std::make_unique<renderer::Renderer>(
                                            std::make_unique<renderer::DeferredRenderingStrategy>())),
                                        window(std::make_unique<engine::Window>(
                                            "Hammock", renderer->getInstance(), 1920u, 1080u)) {
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
        std::unique_ptr<renderer::Renderer> renderer;
        std::unique_ptr<engine::Window> window;
    };
}
