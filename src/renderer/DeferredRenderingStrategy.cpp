module;

#include <memory>
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock_renderer.deferred_rendering_strategy;

import hammock_renderer.rendering_strategy;
import hammock_core;
import hammock_renderer.graphics_context;


hammock::renderer::DeferredRenderingStrategy::DeferredRenderingStrategy(GraphicsContext &ctx, std::uint32_t maxFramesInFlight): IRenderingStrategy(ctx, maxFramesInFlight) {
    core::SwapChain::forEachFrameInFlight([this](int i) {
        auto commandBuffer =  std::make_unique<core::CommandBuffer>(this->ctx_.getDevice(), core::CommandQueueFamily::Graphics);
        commandBuffers_.push_back(std::move(commandBuffer));
    });
}

void hammock::renderer::DeferredRenderingStrategy::draw(core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore &signal) {
    auto &commandBuffer = *commandBuffers_[frameIndex];
    auto image = core::ResourceManager::getInstance().getResource<core::Image>(target);

    commandBuffer.addSignalSemaphore(signal);
    commandBuffer.begin();

    // Attachment needs to be in color attachment optimal layout
    image->recordPipelineBarrier(
        commandBuffer.getCommandBuffer(),
        vk::PipelineStageFlagBits2::eNone,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored
    );

    auto attachment = image->getRenderingAttachmentInfo();
    attachment.loadOp = vk::AttachmentLoadOp::eClear;
    attachment.storeOp = vk::AttachmentStoreOp::eStore;

    vk::RenderingInfo renderInfo{};
    renderInfo.renderArea = vk::Rect2D{{0,  0}, {image->getExtent().width, image->getExtent().height}};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &attachment;

    commandBuffer.getCommandBuffer().beginRendering(&renderInfo);
    commandBuffer.getCommandBuffer().endRendering();

    commandBuffer.submit();
}
