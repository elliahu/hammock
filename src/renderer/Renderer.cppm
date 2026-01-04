module;

#include <memory>
#include <stdexcept>

export module hammock.renderer.renderer;

export import hammock.renderer.rendering_strategy;

import hammock.core;

namespace hammock::renderer {
    export class Renderer {
    public:
        explicit Renderer(std::unique_ptr<IRenderingStrategy> &&strategy =
                                  {});

        void drawFrame(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &wait, core::Semaphore &signal) const;

    private:
        std::unique_ptr<IRenderingStrategy> strategy_{};;
    };
};
