module;

#include <memory>

export module hammock.renderer.renderer;

export import :strategy;
export import :deferred_strategy;

import hammock.core.instance;

namespace hammock::renderer {
    export class Renderer {
    public:
        explicit Renderer(GraphicsContext &ctx,
                          std::unique_ptr<IRenderingStrategy> &&strategy =
                                  {}) : ctx(ctx), strategy(std::move(strategy)) {
        }

        void drawFrame() const {
            if (ctx.supportsPresent()) {
                strategy->draw(ctx, {});
            } else {
                strategy->draw(ctx, {});
            }
        }

    private:
        GraphicsContext &ctx;
        std::unique_ptr<IRenderingStrategy> strategy{};;
    };
};
