module;
export module hammock.renderer.renderer:deferred_strategy;

import :strategy;

namespace hammock::renderer {
    export class DeferredRenderingStrategy : public IRenderingStrategy {
    public:
        void draw() override{}
    };
}