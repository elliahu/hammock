#pragma once
#include <cstdint>

#include "graphics_context.hpp"

namespace hammock::renderer {
    class BaseRenderingStrategy {
    public:
        BaseRenderingStrategy(GraphicsContext &context) : ctx_(context){
        }

        virtual ~BaseRenderingStrategy() = default;

        virtual void draw(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &signal) = 0;

    protected:
        GraphicsContext &ctx_;
    };
}
