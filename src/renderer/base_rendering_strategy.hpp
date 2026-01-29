#pragma once
#include <cstdint>

#include "graphics_context.hpp"
#include "hammock_core.hpp"

namespace hammock::renderer {
    class BaseRenderingStrategy {
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
