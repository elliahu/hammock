module;

#include <memory>

export module hammock.engine.application;

import hammock.engine.runner;
import hammock.renderer;


namespace hammock::engine {
    export class Application final {
    public:
        explicit Application(RunnerMode mode);

        void launch() const;

    private:
        std::unique_ptr<renderer::GraphicsContext> context_;
        std::unique_ptr<renderer::Renderer> renderer_;
        std::unique_ptr<Runner> runner_;
    };
}
