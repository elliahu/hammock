module;


export module hammock.renderer.rendering_strategy;

import hammock.core;
import hammock.renderer.graphics_context;

namespace hammock::renderer {
    export class IRenderingStrategy {
    public:
        IRenderingStrategy(GraphicsContext &context) : ctx_(context) {
        }

        virtual ~IRenderingStrategy() = default;

        virtual void draw(core::ResourceHandle target,core::Semaphore &wait, core::Semaphore &semaphore) = 0;

    protected:
        GraphicsContext &ctx_;
    };
}
