module;

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

export module hammock.renderer.renderer:deferred_strategy;

import :strategy;
import hammock.core;

namespace hammock::renderer {
    export class DeferredRenderingStrategy : public IRenderingStrategy {
    public:
        explicit DeferredRenderingStrategy(GraphicsContext &ctx): IRenderingStrategy(ctx) {
            core::SwapChain::forEachFrameInFlight([this](int i) {
                auto commandBuffer =  std::make_unique<core::CommandBuffer>(this->ctx.getDevice(), core::CommandQueueFamily::Graphics);
                commandBuffers.push_back(std::move(commandBuffer));
            });
        }

        void draw(core::ResourceHandle target,core::Semaphore &wait, core::Semaphore &signal) override {
            auto &sc = core::SwapChainManager::getInstance();
            auto frame = sc.getFrameIndex();
            auto &commandBuffer = *commandBuffers[frame];
            commandBuffer.signalSemaphore(signal);
            commandBuffer.waitOnSemaphore(wait, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

            commandBuffer.begin();

            commandBuffer.submit();
        }

    private:
        std::vector<std::unique_ptr<core::CommandBuffer>> commandBuffers;
    };
}