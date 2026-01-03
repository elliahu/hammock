module;

#include <memory>
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock.renderer.deferred_rendering_strategy;

import hammock.renderer.rendering_strategy;
import hammock.core;
import hammock.renderer.graphics_context;
hammock::renderer::DeferredRenderingStrategy::DeferredRenderingStrategy(GraphicsContext &ctx): IRenderingStrategy(ctx) {
    core::SwapChain::forEachFrameInFlight([this](int i) {
        auto commandBuffer =  std::make_unique<core::CommandBuffer>(this->ctx_.getDevice(), core::CommandQueueFamily::Graphics);
        commandBuffers_.push_back(std::move(commandBuffer));
    });
}

void hammock::renderer::DeferredRenderingStrategy::draw(core::ResourceHandle target, core::Semaphore &wait,
    core::Semaphore &signal) {
    auto &sc = core::SwapChainManager::getInstance();
    auto frame = sc.getFrameIndex();
    auto &commandBuffer = *commandBuffers_[frame];
    commandBuffer.signalSemaphore(signal);
    commandBuffer.waitOnSemaphore(wait, vk::PipelineStageFlagBits2::eFragmentShader);

    commandBuffer.begin();

    commandBuffer.submit();
}
