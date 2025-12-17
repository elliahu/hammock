module;

#include <memory>

export module hammock.renderer.renderer;

export import :strategy;
export import :deferred_strategy;

import hammock.core.instance;

namespace hammock::renderer {
    export class Renderer {
    public:
        explicit Renderer(std::unique_ptr<IRenderingStrategy> &&strategy = {}) : strategy(std::move(strategy)) {}

        void drawFrame() const {
            strategy->draw();
        }

        core::Instance &getInstance() const {return strategy->getInstance();}
    private:
        std::unique_ptr<IRenderingStrategy> strategy{};
    };
};