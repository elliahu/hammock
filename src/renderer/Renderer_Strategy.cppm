module;
export module hammock.renderer.renderer:strategy;

import hammock.core.instance;
import hammock.core.device;

namespace hammock::renderer {
    export class IRenderingStrategy {
    public:
        IRenderingStrategy() = default;

        virtual ~IRenderingStrategy() = default;

        virtual void draw() = 0;

        core::Instance &getInstance() { return instance; }

    private:
        core::Instance instance{};
    };
}
