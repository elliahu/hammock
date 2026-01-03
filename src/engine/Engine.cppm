module;

#include <memory>

export module hammock.engine.engine;

import hammock.engine.editor;
import hammock.renderer;


namespace hammock::engine {
    export class Engine final {
    public:
        explicit Engine(LaunchMode mode);

        void launch() const;

    private:
        std::unique_ptr<renderer::GraphicsContext> context_;
        std::unique_ptr<renderer::Renderer> renderer_;
        std::unique_ptr<Editor> editor_;
    };
}
