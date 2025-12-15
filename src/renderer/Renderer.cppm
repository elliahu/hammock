module;

#include <memory>

export module hammock.renderer.renderer;

import :strategy;

namespace hammock::renderer {
    export class Renderer {
    public:
        explicit Renderer(std::unique_ptr<IRenderingStrategy> &&strategy = {}) : strategy(std::move(strategy)) {}

        void drawFrame() const {
            strategy->draw();
        }
    private:
        std::unique_ptr<IRenderingStrategy> strategy{};
    };
};