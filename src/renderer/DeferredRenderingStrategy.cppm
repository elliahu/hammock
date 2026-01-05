module;

#include <memory>
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock_renderer:deferred_rendering_strategy;

import :rendering_strategy;
import hammock_core;
import :graphics_context;

namespace hammock::renderer {
    export class DeferredRenderingStrategy : public IRenderingStrategy {
    public:
        explicit DeferredRenderingStrategy(GraphicsContext &ctx, std::uint32_t maxFramesInFlight);

        void draw(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &signal) override;

    private:
        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers_;
    };
}