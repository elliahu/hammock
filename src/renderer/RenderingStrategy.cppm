module;

#include <cstdint>

export module hammock.renderer.rendering_strategy;

import hammock.core;
import hammock.renderer.graphics_context;

namespace hammock::renderer {
    export class IRenderingStrategy {
    public:
        IRenderingStrategy(GraphicsContext &context, std::uint32_t maxFramesInFlight) : ctx_(context), maxFramesInFlight_(maxFramesInFlight) {
        }

        virtual ~IRenderingStrategy() = default;

        virtual void draw(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &wait, core::Semaphore &signal) = 0;

    protected:
        GraphicsContext &ctx_;
        std::uint32_t maxFramesInFlight_;
    };
}
