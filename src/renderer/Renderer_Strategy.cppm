module;


export module hammock.renderer.renderer:strategy;

import hammock.core;
import hammock.renderer.graphics_context;

namespace hammock::renderer {
    export class IRenderingStrategy {
    public:
        IRenderingStrategy(GraphicsContext &context) : ctx(context) {
        }

        virtual ~IRenderingStrategy() = default;

        virtual void draw(core::ResourceHandle target,core::Semaphore &wait, core::Semaphore &semaphore) = 0;

    protected:
        GraphicsContext &ctx;
    };
}
