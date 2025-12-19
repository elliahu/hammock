module;

#include <memory>
#include <stdexcept>

export module hammock.renderer.renderer;

export import :strategy;
export import :deferred_strategy;

import hammock.core;

namespace hammock::renderer {
    export class Renderer {
    public:
        explicit Renderer(std::unique_ptr<IRenderingStrategy> &&strategy =
                                  {}) : strategy(std::move(strategy)) {
        }

        void drawFrame(core::ResourceHandle target, core::Semaphore &wait, core::Semaphore &signal) const {
            if (!target.isValid() || target.getType() != core::ResourceType::Image) {
                throw std::runtime_error("Invalid rendering target");
            }

            strategy->draw(target, wait, signal);
        }

    private:
        std::unique_ptr<IRenderingStrategy> strategy{};;
    };
};
