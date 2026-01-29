#pragma once
#include <memory>
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "base_rendering_strategy.hpp"
#include "graphics_context.hpp"
#include "hammock_core.hpp"


namespace hammock::renderer {
    class DeferredRenderingStrategy : public BaseRenderingStrategy {
    public:
        explicit DeferredRenderingStrategy(GraphicsContext &ctx, std::uint32_t maxFramesInFlight);

        void draw(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &signal) override;

    private:
        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers_;
    };
}