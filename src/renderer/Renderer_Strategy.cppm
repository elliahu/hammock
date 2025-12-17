module;


export module hammock.renderer.renderer:strategy;

import hammock.core.instance;
import hammock.core.device;
import hammock.core.resource_manager;
import hammock.renderer.graphics_context;

namespace hammock::renderer {

    export class RenderingContext {

    };

    export class IRenderingStrategy {
    public:
        IRenderingStrategy() = default;
        virtual ~IRenderingStrategy() = default;
        virtual void draw(GraphicsContext& gctx, const RenderingContext& rctx) = 0;

    private:
    };
}
