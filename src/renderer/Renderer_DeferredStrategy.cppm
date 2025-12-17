module;
export module hammock.renderer.renderer:deferred_strategy;

import :strategy;

namespace hammock::renderer {
    export class DeferredRenderingStrategy : public IRenderingStrategy {
    public:
        explicit DeferredRenderingStrategy(GraphicsContext &ctx): IRenderingStrategy() {
        }

        void draw(GraphicsContext& ctx, const RenderingContext & rctx) override{}
    };
}