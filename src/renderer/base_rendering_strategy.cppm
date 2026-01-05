module;

#include <cstdint>

export module hammock_renderer:rendering_strategy;

import hammock_core;
import :graphics_context;

namespace hammock::renderer {
    export class BaseRenderingStrategy {
    public:
        BaseRenderingStrategy(GraphicsContext &context, std::uint32_t maxFramesInFlight) : ctx_(context), maxFramesInFlight_(maxFramesInFlight) {
        }

        virtual ~BaseRenderingStrategy() = default;

        virtual void draw(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &signal) = 0;

    protected:
        GraphicsContext &ctx_;
        const std::uint32_t maxFramesInFlight_;
    };
}
