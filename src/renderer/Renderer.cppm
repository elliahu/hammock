module;

#include <memory>
#include <stdexcept>

export module hammock_renderer.renderer;

export import hammock_renderer.rendering_strategy;

import hammock_core;

namespace hammock::renderer {
    export class Renderer {
    public:
        explicit Renderer(std::unique_ptr<IRenderingStrategy> &&strategy =
                                  {});

        void drawFrame(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &signal) const;

    private:
        std::unique_ptr<IRenderingStrategy> strategy_{};;
    };
};
