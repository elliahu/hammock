module;

#include <memory>

export module hammock.engine.engine;

import hammock.engine.editor;
import hammock.renderer;


namespace hammock::engine {
    export class Engine final {
    public:
        Engine(LaunchMode mode) {
            // Create graphics context
            context = std::make_unique<renderer::GraphicsContext>(renderer::GraphicsContextDesc{
                .mode = renderer::GraphicsContextMode::Windowed,
            });

            // Create editor
            editor = std::make_unique<Editor>(mode, *context);
        }

        void launch() const {
            editor->launch();
        }

    private:
        std::unique_ptr<renderer::GraphicsContext> context;
        std::unique_ptr<renderer::Renderer> renderer;
        std::unique_ptr<Editor> editor;
    };
}
